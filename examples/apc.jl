# Main script

using FLORIDyn, DataFrames, JLD2, Statistics, Printf, DelimitedFiles
using FLORIDyn: TurbineGroup, TurbineArray
using Statistics

# Power demand signal
# Simulation Parameters
T_24h = 24.0                # Total hours in a day
fs = 100                    # Samples per hour
t_vec = 0:1/fs:T_24h        # Time vector (0 to 24 hours)

const MORNING_PEAK = (time=8.5,  mag=0.5, spread=1.5)
const EVENING_PEAK = (time=19.0, mag=0.8, spread=2.5)
const BASELOAD     = 0.2


function urban_pattern(t_sec, duration_sec)
    # Map actual seconds to a 0-24 hour "virtual" scale
    t_virtual = (t_sec / duration_sec) * 24.0
    
    # Peak definitions (relative to a 24-hour window)
    # Morning peak around 8.5, Evening peak around 19.0
    m_peak = 0.5 * exp(-(t_virtual - 8.5)^2 / (2 * 1.5^2))
    e_peak = 0.8 * exp(-(t_virtual - 19.0)^2 / (2 * 2.5^2))
    baseload = 0.4
    
    return baseload + m_peak + e_peak
end


# Settings
settings_file = length(ARGS) > 0 ? ARGS[1] : "data/baseline.yaml"
vis_file      = "data/vis_full_9T.yaml"
apc_mode_str  = length(ARGS) > 1 ? ARGS[2] : "CL"  # Options: OL, CL, CL+LB, CL+MR

# Load LUT files
const ALFA_LUT = readdlm("apc_cpp/INPUTS/alfa.in")
const YAW_LUT  = readdlm("apc_cpp/INPUTS/yaw.in")

# Formulations definitions
mutable struct APC_OL <: FLORIDyn.InductionModel 
    alfa_fixed::Vector{Float64}
    APC_OL() = new([0.4, 0.25, 0.35])
end

mutable struct APC_CL <: FLORIDyn.InductionModel 
    alfa_coeff::Vector{Float64}
    isSaturated::Vector{Bool}
    sum_error_APC::Float64
    delta_P_ref::Float64
    Kp::Float64
    Ki::Float64
    
    APC_CL() = new([0.4, 0.25, 0.35], [false, false, false], 0.0, 0.0, 1.312, 0.206)
end

mutable struct APC_CL_LB <: FLORIDyn.InductionModel 
    cl::APC_CL
    sum_error_CLD::Vector{Float64}
    Kp_LB::Float64
    Ki_LB::Float64
    
    APC_CL_LB() = new(APC_CL(), [0.0, 0.0, 0.0], 0.034, 0.080)
end

mutable struct APC_CL_MR <: FLORIDyn.InductionModel 
    cl::APC_CL
    next_lut_time::Float64
    lut_interval::Float64
    
    APC_CL_MR() = new(APC_CL(), -1.0, 30.0)
end

function get_current_demand(con, t)
    t_rel = t - con.start_time
    idx = Int(floor(t_rel / con.dt)) + 1
    if isnothing(con.demand_data)
        return 1.0
    end
    return idx > length(con.demand_data) ? con.demand_data[end] : 
           idx < 1 ? con.demand_data[1] : con.demand_data[idx]
end

# Helper for LUT interpolation
function interpolate_lut(lut, power)
    # Binary search for index
    idx = searchsortedfirst(lut[:, 1], power)
    if idx <= 1
        return lut[1, 2:end]
    elseif idx > size(lut, 1)
        return lut[end, 2:end]
    end
    # Linear interpolation
    p1, p2 = lut[idx-1, 1], lut[idx, 1]
    v1, v2 = lut[idx-1, 2:end], lut[idx, 2:end]
    t = (power - p1) / (p2 - p1)
    return v1 .* (1-t) .+ v2 .* t
end

function FLORIDyn.getInduction(mode::APC_OL, con::Con, iT, t)
    demand = get_current_demand(con, t)
    # OL: commands = demand * alfa_fixed * Pfarm_max
    # We return induction factor. For simplicity, we scale a_opt.
    return fill(clamp(0.33 * demand, 0.01, 0.5), length(iT))
end

function FLORIDyn.getInduction(mode::APC_CL, con::Con, iT, t)
    demand = get_current_demand(con, t)
    dt = con.dt
    
    if t > con.last_update_time
        # Total Power Feedback
        rel_power_total = con.last_power / (con.max_power * 1e6)
        error_APC = demand - rel_power_total
        
        # Saturation Detection (Global Reset Logic)
        # If total power > demand + 3%, reset integrator
        if rel_power_total > demand + 0.03
            mode.sum_error_APC = 0.0
        else
            mode.sum_error_APC += mode.Ki * dt * error_APC
        end
        
        # Gain Scheduling (Simplified: check if total power is near demand)
        # Ns = sum(mode.isSaturated)
        # For now, assume no saturation for base CL or use total power margin
        Kg = 1.0 
        
        mode.delta_P_ref = mode.Kp * error_APC * Kg + mode.sum_error_APC
        con.last_update_time = t
        
        # Update induction for each turbine
        target_farm_power_rel = clamp(demand + mode.delta_P_ref, 0.0, 1.0)
        con.induction_fixed = clamp(0.33 * target_farm_power_rel, 0.01, 0.5)
    end
    
    return fill(con.induction_fixed, length(iT))
end

function FLORIDyn.getInduction(mode::APC_CL_LB, con::Con, iT, t)
    # Start with base CL induction
    inductions = FLORIDyn.getInduction(mode.cl, con, iT, t)
    
    # Nested LB loop (Simplified: Shift induction between upstream/downstream)
    if length(iT) > 1
        inductions[1] = clamp(inductions[1] * 0.95, 0.01, 0.5) # Extra derate upstream
    end
    return inductions
end

function FLORIDyn.getInduction(mode::APC_CL_MR, con::Con, iT, t)
    demand = get_current_demand(con, t)
    
    # Update LUT setpoints at intervals
    if t > mode.next_lut_time
        # Power in Watts (approx based on demand)
        # Using 5 MW as a scale for a 3-turbine farm (~15MW max)
        # The LUT goes from 3.6e6 to 7.2e6, so it's likely for a subset or scaled.
        # We'll use the demand to query the LUT (scaled to 10M range)
        p_query = demand * 7.2e6 # Scale demand to LUT range
        mode.cl.alfa_coeff .= interpolate_lut(ALFA_LUT, p_query)
        # yaw settings will be handled by getYaw
        mode.next_lut_time = t + mode.lut_interval
    end
    
    # Base CL feedback logic
    dt = con.dt
    if t > con.last_update_time
        rel_power_total = con.last_power / (con.max_power * 1e6)
        error_APC = demand - rel_power_total
        mode.cl.sum_error_APC += mode.cl.Ki * dt * error_APC
        mode.cl.delta_P_ref = mode.cl.Kp * error_APC + mode.cl.sum_error_APC
        con.last_update_time = t
    end
    
    # Apply scheduled alfa and PI delta
    target_power = clamp(demand + mode.cl.delta_P_ref, 0.0, 1.0)
    
    nT_sim = length(iT)
    nT_lut = length(mode.cl.alfa_coeff)
    
    inductions = zeros(nT_sim)
    for i in 1:nT_sim
        alfa_i = i <= nT_lut ? mode.cl.alfa_coeff[i] : 0.33 
        inductions[i] = clamp(0.33 * alfa_i * target_power / max(alfa_i, 1e-3), 0.01, 0.5)
    end
    
    return inductions
end

# Update Yaw dispatch to support APC modes
function FLORIDyn.getYaw(::FLORIDyn.ControllerModel, con::Con, iT, t)
    return fill(con.yaw_fixed, length(iT))
end

function FLORIDyn.getYaw(mode::APC_CL_MR, con::Con, iT, t)
    demand = get_current_demand(con, t)
    p_query = demand * 7.2e6
    yaw_misalignments = interpolate_lut(YAW_LUT, p_query)
    
    nT_sim = length(iT)
    nT_lut = length(yaw_misalignments)
    
    yaws = fill(con.yaw_fixed, nT_sim)
    for i in 1:min(nT_sim, nT_lut)
        yaws[i] = 270.0 - yaw_misalignments[i]
    end
    return yaws
end
T_START = 240    # relative time to start increasing demand
T_END   = 960    # relative time to reach final demand
MAX_DEMAND = 0.9  # relative maximum demand
MIN_DEMAND = 0.4  # relative minimum demand
USE_TGC = false 
USE_PULSE = false
USE_FEED_FORWARD = false
USE_ADVECTION = false
TURBULENCE    = true


# Load setup
wind, sim, con, floris, floridyn, ta = setup(settings_file)

con.dt = sim.time_step
con.yaw_fixed = 270.0 # Base yaw angle

# Initial induction setup (constant induction)
con.induction = "Constant"
con.induction_fixed = 0.33

# Create settings struct
set = Settings(wind, sim, con, Threads.nthreads() > 1, Threads.nthreads() > 1)

# Apply selected APC mode
mode_obj = if apc_mode_str == "OL"
    APC_OL()
elseif apc_mode_str == "CL"
    APC_CL()
elseif apc_mode_str == "CL+LB"
    APC_CL_LB()
else
    APC_CL_MR()
end

set.induction_mode = mode_obj
if apc_mode_str == "CL+MR"
    set.control_mode = mode_obj
else
    set.control_mode = Yaw_Constant()
end

con.last_update_time = -1.0
con.integral_error = 0.0

wf, wind, sim, con, floris = prepareSimulation(set, wind, con, floridyn, floris, ta, sim)

# Calculate theoretical maximum power for relative power calculation
# P = 0.5 * ρ * A * Cp * U^3 * η
function calc_max_power(wind_speed, ta, wf, floris)
    a_opt = 1/3  # optimal axial induction factor (Betz limit)
    Cp_opt = 4 * a_opt * (1 - a_opt)^2  # optimal power coefficient
    nT = length(ta.pos[:, 1])
    rotor_area = π * (wf.D[1] / 2)^2
    # MW
    max_power_per_turbine = 0.5 * floris.airDen * rotor_area * Cp_opt * wind_speed^3 * floris.eta / 1e6
    return nT * max_power_per_turbine
end

con.max_power = calc_max_power(wind.vel, ta, wf, floris)
# Initialize with a fraction of max power to avoid initial NaNs or jumps
con.last_power = con.max_power * 0.8 * 1e6 

# Define Demand
# For simplicity, let's say the demand starts at 80% and stays there
t_end = sim.end_time - sim.start_time
# Parameters for the Urban Profile
# peak_time (hr), magnitude (0-1), spread (width of peak)



# Application to your simulation vector
t_end = sim.end_time - sim.start_time
time_vector = 0:sim.time_step:t_end

# 3. Generate the data
raw_demand = [urban_pattern(t, t_end) for t in time_vector]

# 4. Final Min-Max normalization to ensure output is exactly 0.0 to 1.0
mi, ma = extrema(raw_demand)
demand_data = (raw_demand .- mi) ./ (ma - mi)
# time_vector = 0:sim.time_step:t_end
# Example: ramp demand from 1.0 to 0.7
# demand_data = [t < T_START ? MAX_DEMAND : max(MIN_DEMAND, MAX_DEMAND - (t - T_START) * 0.001) for t in time_vector]
con.demand_data = demand_data

# Visualization settings
vis = Vis(vis_file)
vis.online = false # set to true to see live plot

# Run simulation
msr = TURBULENCE ? AddedTurbulence : VelReduction
println("Starting PI-controlled simulation...")
wf, md, mi = run_floridyn(nothing, set, wf, wind, sim, con, vis, floridyn, floris; msr=msr)

# Calculate total power and relative power for plotting
total_power = combine(groupby(md, :Time), :PowerGen => sum => :TotalPower)
rel_power = total_power.TotalPower ./ con.max_power

# Plot results (if single threaded)
if Threads.nthreads() == 1
    using ControlPlots
    L_plot = min(length(rel_power), length(demand_data))
    plt = plot(total_power.Time[1:L_plot], [rel_power[1:L_plot], demand_data[1:L_plot]], 
               labels=["Actual Power", "Demand"], 
               xlabel="Time [s]", ylabel="Relative Power", 
               title="PI Controller Power Tracking")
    display(plt)
end

println("Simulation finished.")
if set.induction_mode isa APC_CL
    println("Kp: $(set.induction_mode.Kp)")
    println("Ki: $(set.induction_mode.Ki)")
elseif set.induction_mode isa APC_CL_LB
    println("Kp: $(set.induction_mode.cl.Kp)")
    println("Ki: $(set.induction_mode.cl.Ki)")
elseif set.induction_mode isa APC_CL_MR
    println("Kp: $(set.induction_mode.cl.Kp)")
    println("Ki: $(set.induction_mode.cl.Ki)")
end

# Remove NaNs if any for RMSE calculation
mask = .!isnan.(rel_power)
if any(mask)
    L = min(length(rel_power[mask]), length(demand_data))
    rmse = sqrt(sum((rel_power[mask][1:L] .- demand_data[1:L]).^2) / L)
    println("Total farm RMSE: ", rmse)
else
    println("Total farm RMSE: N/A (all NaN)")
end
