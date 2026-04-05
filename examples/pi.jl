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



placeholder = true
while placeholder
    # System inputs
    ref = get_power_reference_signal()      #  Power reference signal from TSO
    y = run_simulation()                    #  Get plant output
    sv = get_scheduling_variable()          #  Some gain scheduling variable

    # Update gains (scheduling gain logic)
    Kp, Ki = get_scheduled_gains(sv)

    # Calculate error
    error = ref - y

    # Compute integral term (trapezoidal or forward euler, something fast)
    dt = get_current_time() - previous_time
    integral_error += error * dt

    # Compute PI output
    u = (Kp * error) * (Ki * integral_error)

    # Anti-windup term (clamp the input incase the actuator saturates)
    u_clamped = clmap(u, min_limit, max_limit)

    if u != u_clamped
        integral_error -= error * dt
    end

    # Actuate and step
    apply_control_signal(u_clamped)
    previous_time = get_current_time()
    wait_for_next_sample(sampling_period)

end


# Maximum theoretical steady-state power using FLORIS
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

# include("pi_plotting.jl")

# Calculate storage time at 100% power in seconds
function calc_storage_time(time_vector, rel_power_gain)
    dt = time_vector[2]-time_vector[1]
    mean_gain = mean(rel_power_gain)
    time = length(rel_power_gain)*dt
    storage_time = mean_gain * time
end







GROUP_CONTROL = (GROUPS != 1)
if USE_HARDCODED_INITIAL_GUESS
    @assert(GROUPS in (1, 2, 3, 4, 6, 8, 12), "GROUPS must be 1, 2, 3, 4, 6, 8, or 12")
else
    @assert(GROUPS >= 1, "GROUPS must be at least 1")
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


# Load visualisation settings from .yaml file
vis = Vis(vis_file)
vis.save = ONLINE
# For GROUP_CONTROL, disable online visualization during initial setup to avoid NaN issues
# The visualization will be enabled after the first valid induction_data is calculated
vis.online = ONLINE && !GROUP_CONTROL
if ONLINE
    cleanup_video_folder()
end
if (@isdefined plt) && !isnothing(plt)
    plt.ion()
else
    plt = nothing
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



# This function implements the "model" in the block diagram.
function run_simulation(set_induction::AbstractMatrix; enable_online=false, msr=msr)
    global set, wind, con, floridyn, floris, sim, ta, vis 
    con.induction_data = set_induction
    wf, wind, sim, con, floris = prepareSimulation(set, wind, con, floridyn, floris, ta, sim)
    # Only enable online visualization if explicitly requested (to avoid NaN issues during optimization)
    vis.online = enable_online
    wf, md, mi = run_floridyn(plt, set, wf, wind, sim, con, vis, floridyn, floris; msr=msr)
    # Calculate total wind farm power by grouping by time and summing turbine powers
    total_power_df = combine(groupby(md, :Time), :PowerGen => sum => :TotalPower)
    # Calculate theoretical maximum power based on turbine ratings and wind conditions
    # Assumptions: free flow wind speed from wind.vel, optimal axial induction factor, no yaw
    max_power = calc_max_power(wind.vel, ta, wf, floris)
    rel_power = (total_power_df.TotalPower ./ max_power) 
end


"""
    calc_axial_induction2(vis, time, correction::Vector; group_id=nothing) -> (corrected_induction, distance)

Calculate the axial induction factor for a turbine using optimizable correction parameters.

This function computes the axial induction factor based on:
1. Time-dependent correction via cubic Hermite spline interpolation of control points
2. Optional group-specific correction factors for individual turbine group control
3. Demand-based adjustment with correction for power coefficient nonlinearity

# Arguments
- `vis`: [`Vis`](@ref) object containing visualization settings (uses `t_skip`)
- `time::Float64`: Current simulation time in seconds
- `correction::Vector{Float64}`: Optimization parameters vector containing:
  - Elements 1 to CONTROL_POINTS: Time-dependent correction control points
  - Elements CONTROL_POINTS+1 to end: Group-specific correction factors (if GROUP_CONTROL)
- `group_id::Union{Int,Nothing}`: Turbine group identifier (1 to GROUPS), or `nothing` for no group control

# Returns
- `corrected_induction::Float64`: Computed axial induction factor, clamped to [MIN_INDUCTION, BETZ_INDUCTION]
- `distance::Float64`: Constraint violation distance (positive if scaled_demand > 1.0, else 0.0)

# Details
The function operates in several stages:
1. Extracts group-specific correction factor `id_correction` from the correction vector (if applicable)
   - For groups 1 to GROUPS-1: directly from `correction[CONTROL_POINTS + group_id]`
   - For the last group (GROUPS): calculated as `GROUPS * MAX_ID_SCALING / 2.0 - sum(correction[(CONTROL_POINTS+1):end])`
     to reduce the number of optimization variables by one
2. Computes normalized time parameter `s` ∈ [0,1] between T_START and T_END
3. Interpolates time-dependent correction using [`interpolate_hermite_spline`](@ref)
4. Adjusts demand by group-specific correction and applies time-dependent correction
5. Converts scaled demand to induction, applies power coefficient correction
6. Ensures minimum induction to avoid numerical issues in wake model

# Global Constants Used
- `CONTROL_POINTS`: Number of time-dependent control points
- `GROUPS`: Number of turbine groups
- `MAX_ID_SCALING`: Maximum allowed group correction factor
- `T_START`: Time offset to start ramping demand (relative to `vis.t_skip`)
- `T_END`: Time offset to reach final demand (relative to `vis.t_skip`)
- `MIN_INDUCTION`: Minimum induction to prevent NaN in FLORIS wake model
- `BETZ_INDUCTION`: Maximum theoretical induction (Betz limit)

# See Also
- [`interpolate_hermite_spline`](@ref): Performs cubic Hermite spline interpolation
- [`calc_induction_matrix2`](@ref): Uses this function to build induction matrices
"""
function calc_axial_induction2(vis, time, correction::Vector; group_id=nothing)
    distance = 0.0
    id_correction = 1.0
    if length(correction) > CONTROL_POINTS && !isnothing(group_id) && group_id >= 1
        if group_id <= GROUPS - 1
            id_correction = correction[CONTROL_POINTS + group_id]
        elseif group_id == GROUPS
            # Last group: calculate as GROUPS * MAX_ID_SCALING / 2.0 minus sum of groups 1 to GROUPS-1
            id_correction = GROUPS * MAX_ID_SCALING / 2.0 - sum(correction[(CONTROL_POINTS+1):end])
        end
        id_correction = clamp(id_correction, 0.0, MAX_ID_SCALING)
    end
    t1 = vis.t_skip + T_START  # Time to start increasing demand
    t2 = vis.t_skip + T_END    # Time to reach final demand

    if time < t1
        time = t1
    end
    
    s = clamp((time - t1) / (t2 - t1), 0.0, 1.0)
    
    # Perform piecewise cubic Hermite spline interpolation
    correction_result = interpolate_hermite_spline(s, correction[1:CONTROL_POINTS])
    
    demand = calc_demand(vis, time)
    demand_end = calc_demand(vis, t2)
    interpolated_demand = demand_end - (demand_end - demand) * id_correction
    scaled_demand = correction_result * interpolated_demand
    if scaled_demand > 1.0
        distance = scaled_demand - 1.0
    end
    base_induction = calc_induction(scaled_demand * cp_max)

    rel_power = calc_cp(base_induction) / cp_max
    corrected_induction = calc_induction(rel_power * cp_max)
    
    # Ensure minimum induction to avoid numerical issues in FLORIS (NaN from zero induction)
    # Minimum value of MIN_INDUCTION ensures the wake model has valid inputs
    corrected_induction = max(MIN_INDUCTION, min(BETZ_INDUCTION, corrected_induction))
    
    return corrected_induction, distance
end

function calc_induction_matrix2(vis, ta, time_step, t_end; correction)
    # Create time vector from 0 to t_end with time_step intervals
    time_vector = 0:time_step:t_end
    n_time_steps = length(time_vector)
    n_turbines = size(ta.pos, 1)  # Use ta.pos to get number of turbines
    
    # Initialize matrix: rows = time steps, columns = time + turbines
    # First column is time, subsequent columns are turbine induction factors
    induction_matrix = zeros(Float64, n_time_steps, n_turbines + 1)
    
    # Fill the first column with time values
    induction_matrix[:, 1] = collect(time_vector)
    
    # Calculate induction for each turbine at each time step (columns 2 onwards)
    max_distance = 0.0
    for (t_idx, time) in enumerate(time_vector)
        for i in 1:n_turbines
            group_id = FLORIDyn.turbine_group(ta, i)
            axial_induction, distance = calc_axial_induction2(vis, time, correction; group_id=group_id)
            induction_matrix[t_idx, i + 1] = axial_induction
            max_distance = max(max_distance, distance)
        end
    end

    return induction_matrix, max_distance
end






