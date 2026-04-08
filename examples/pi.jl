# PI controller example for FLORIDyn.jl

using FLORIDyn, DataFrames, JLD2, Statistics, Printf
using FLORIDyn: TurbineGroup, TurbineArray

# Settings
settings_file = "data/baseline.yaml"
vis_file      = "data/vis_full_9T.yaml"
T_START = 240    # relative time to start increasing demand
T_END   = 960    # relative time to reach final demand
MAX_DEMAND = 0.9  # relative maximum demand
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
con.last_power = con.max_power # Initialize with max power to avoid initial jump

# Define Demand
# For simplicity, let's say the demand starts at 80% and stays there
t_end = sim.end_time - sim.start_time
time_vector = 0:sim.time_step:t_end
# Example: ramp demand from 1.0 to 0.7
demand_data = [t < T_START ? MAX_DEMAND : max(0.4, MAX_DEMAND - (t - T_START) * 0.001) for t in time_vector]
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
    plt = plot(total_power.Time, [rel_power, demand_data[1:length(rel_power)]], 
               labels=["Actual Power", "Demand"], 
               xlabel="Time [s]", ylabel="Relative Power", 
               title="PI Controller Power Tracking")
    display(plt)
end

println("Simulation finished.")

rmse = sqrt(sum((rel_power .- demand_data[1:length(rel_power)]).^2) / length(rel_power))
println("Total farm RMSE: ", rmse)
