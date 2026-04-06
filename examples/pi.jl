# leave space for comments n stuff



using Pkg

# Imports
using FLORIDyn, TerminalPager, DistributedNext, DataFrames, JLD2, Statistics, Printf
using FLORIDyn, TurbineGroup, TurbineArray

# Serial vs multi-threaded
if Threads.nthreads() == 1; using ControlPlots; end

settings_file = "data/baseline.yaml" 
vis_file = "data/vis_full_9T.yaml"
data_file = "data/mpc_result_9.jld2"
error_file = "data/mpc_error_9.jld2"
data_file_group_control = "data/mpc_result_group_control"

GROUPS = 3
CONTROL_POINTS = 5
MAX_ID_SCALING = 3.0
FINAL_DEMAND = 0.8
MAX_STEPS = 1
USE_HARDCODED_INITIAL_GUESS = true
USE_TGC = false
USE_STEP = false
USE_PULSE = false
USE_FEED_FORWARD = true
ONLINE = false
TURBULENCE = true
USE_ADVECTION = false
T_START = 240
T_END = 960
T_EXTRA = 2580

MIN_INDUCTION = 0.01
MAX_DISTANCES = Float64[]
data_file_group_control = data_file_group_control * '_' * string(GROUPS) * "TGs.jld2"


# INITIALISE SYSTEM PARAMETERS
integral_error = 0.0
previous_time = time()
sampling_period = 2.0

# GAIN SCHEDULE
function get_scheduled_gains(scheduling_var)
# figure out how to implement this but just use constant gains for now
end

function get_scheduling_variable()
# May need this
end


# California Independent System Operator (CAISO)
function caiso_score(Pgen::Float64, Pagc_current::Float64, Pagc_previous::Float64, Ps::Float64)
    # Total command power
    P_cmd = Ps - Pagc_current

    # AGC tracking error 
    E = Pgen - Pcmd

    # Mileage
    M = abs(Pagc_current - Pagc_previous)

    S_A = max(0, 1 - (abs(E)/M))

end


# SIMULATION BLOCK: acts as the plant block (where we get out model output)
function get_plant_output(set_induction::AbstractMatrix; enable_online=false, msr=msr)
    global set, wind, con, floridyn, floris, sim, ta, Vis
    con.induction_data = set_induction!
    wf, wind, sim, con, floris = prepareSimulation(set, wind, con, floridyn, floris, ta, sim)

    # Only enable online vis if explicity requested (to avoid NaN issues during optimisation)
    vis.online = enable_online
    wf, md, mi = run_floridyn(plt, set, wf, wnd, sim, con, vis, floridyn, floris; msr=msr)

    # Calculate total wind farm power by grouping time and summing turbine powers
    total_power_df = combine(groupby(md, :Time), :PowerGen => sum => :TotalPower)

    # Calculate theoretical maximum power based on turbine ratings and wind conditions
    # Assumptions: free flow wind speed from wind.vel, optimal axial induction factor, no yaw
    max_power = calc_max_power(wind.vel, ta, wf, floris)
    rel_power = (total_power_df.TotalPpower ./ max_power)
end



# placeholder = true
# while placeholder
#     # System inputs
#     ref = get_power_reference_signal()      #  Power reference signal from TSO
#     y = run_simulation()                    #  Get plant output
#     sv = get_scheduling_variable()          #  Some gain scheduling variable

#     # Update gains (scheduling gain logic)
#     Kp, Ki = get_scheduled_gains(sv)

#     # Calculate error
#     error = ref - y

#     # Compute integral term (trapezoidal or forward euler, something fast)
#     dt = get_current_time() - previous_time
#     integral_error += error * dt

#     # Compute PI output
#     u = (Kp * error) * (Ki * integral_error)

#     # Anti-windup term (clamp the input incase the actuator saturates)
#     u_clamped = clmap(u, min_limit, max_limit)

#     if u != u_clamped
#         integral_error -= error * dt
#     end

#     # Actuate and step
#     apply_control_signal(u_clamped)
#     previous_time = get_current_time()
#     wait_for_next_sample(sampling_period)

# end


"""
    calc_max_power(wind_speed, ta, wf, floris) -> Float64

Calculate the theoretical maximum power output for the wind farm.

# Arguments
- `wind_speed::Float64`: Free flow wind speed in m/s
- `ta::TurbineArray`: Turbine array containing position data
- `wf::WindFarm`: Wind farm object containing rotor diameter data
- `floris::Floris`: FLORIS parameters containing air density and efficiency

# Returns
- `max_power::Float64`: Maximum total power in MW

# Assumptions
- Optimal axial induction factor (Betz limit: a = 1/3)
- No yaw misalignment (yaw = 0°)
- All turbines have the same rotor diameter
"""
function calc_max_power(wind_speed, ta, wf, floris)
    a_opt = 1/3  # optimal axial induction factor (Betz limit)
    Cp_opt = 4 * a_opt * (1 - a_opt)^2  # optimal power coefficient
    yaw = 0.0  # no yaw angle

    # Calculate maximum power for all turbines using getPower formula
    # P = 0.5 * ρ * A * Cp * U^3 * η * cos(yaw)^p_p
    nT = length(ta.pos[:, 1])  # number of turbines
    rotor_area = π * (wf.D[1] / 2)^2  # assuming all turbines have same diameter
    max_power_per_turbine = 0.5 * floris.airDen * rotor_area * Cp_opt * wind_speed^3 * floris.eta * cos(yaw)^floris.p_p / 1e6  # MW
    max_power = nT * max_power_per_turbine  # total maximum power in MW
end


function calc_error(vis, rel_power, demand_data, time_step)
    # Start index after skipping initial transient; +1 because Julia is 1-based
    i0 = Int(floor(vis.t_skip / time_step)) + 1
    # Clamp to valid range
    i0 = max(1, i0)
    n = min(length(rel_power), length(demand_data)) - i0 + 1
    if n <= 0
        error("calc_error: empty overlap after skip; check vis.t_skip and lengths (rel_power=$(length(rel_power)), demand=$(length(demand_data)), i0=$(i0))")
    end
    r = @view rel_power[i0:i0 + n - 1]
    d = @view demand_data[i0:i0 + n - 1]
    return sum((r .- d) .^ 2) / length(d)
end


if MAX_STEPS == 0
   SIMULATE = false # if false, load cached results if available
else
   SIMULATE = true
end


if TURBULENCE
    msr = AddedTurbulence
else
    msr = VelReduction
end



pltctrl = nothing
# Provide ControlPlots module only for pure sequential plotting (single-threaded, no workers)
if Threads.nthreads() == 1
    pltctrl = ControlPlots
end


# Automatic parallel/threading setup
include("remote_plotting.jl")
include("calc_induction_matrix.jl")


# get the settings for the wind field, simulator and controller
wind, sim, con, floris, floridyn, ta = setup(settings_file)


# Override with n groups if GROUPS != 4 (default in settings file is 4)
if GROUPS != 4
    println("Creating $GROUPS turbine groups based on X coordinates...")
    ta = create_n_groups(ta, GROUPS)
end


sim.end_time += T_EXTRA  # extend simulation time for MPC
con.yaw="Constant"
con.yaw_fixed = 270.0
wind.input_dir="Constant"
wind.dir_fixed = 270.0
induction = calc_induction_per_group(vis, 1, 0)
set_induction!(ta, induction)


# For initial setup, use calc_induction_matrix (only affects pre-optimization visualization)
# During optimization, calc_induction_matrix2 will be used with proper group handling
con.induction_data = calc_induction_matrix(vis, ta, time_step, t_end)

# create settings struct with automatic parallel/threading detection
set = Settings(wind, sim, con, Threads.nthreads() > 1, Threads.nthreads() > 1)
if USE_FEED_FORWARD
    set.induction_mode = Induction_TGC()
else
    set.induction_mode = Induction_Constant()
end
wf, wind, sim, con, floris = prepareSimulation(set, wind, con, floridyn, floris, ta, sim)

# Calculate demand for each time point
time_vector = 0:time_step:t_end
demand_data = [calc_demand(vis, t) for t in time_vector]
con.demand_data = demand_data






