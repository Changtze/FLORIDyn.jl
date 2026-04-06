using ControlSystemsBase, Plots
using ControlSystems

G = tf(1, [1, 1, 1])
res = step(G, 20)   # Simulate 20 seconds step response
plot(res)
