# Control Loops


## Fast Control loop
All the FOC gets done in the fastLoop. This is the only critical part of the MESC, the rest can actually be removed providing the parameters get set and MESC is initialised correctly .
If you remove the slowloop, you can write directly to mtr[n]->FOC.Idq_req.q, set the mtr[n]->MotorState to MOTOR_STATE_RUN and enable the inverter.

### The geometric transforms
The forward transforms take place in the ADCConversion(_motor) function. This converts the read currents firstly from 3 phase to two phase (Clarke) and then rotates them to the estimated/read from encoder rotor reference frame (Park).

Geometric transforms are the Clark and Park, which take the forward form are used to transform currents:

Clarke:
\\[\begin{bmatrix}I_\alpha \cr I_\beta \cr I_\gamma \end{bmatrix} = 2/3 \begin{bmatrix} 1 & -0.5 & -0.5\cr0 & \sqrt{3}/2 & -\sqrt{3}/2 \cr 0.5 & 0.5 & 0.5 \end{bmatrix} \begin{bmatrix}I_u \cr I_v \cr I_w \end{bmatrix}  \\]
where MESC selects for the lower of the PWM values at high modulation using the substitution \\( I_u + I_v + I_w = 0\\) .

Park (rotation matrix around \\( \gamma\\)):
\\[ \begin{bmatrix}I_d \cr I_q \cr I_0 \end{bmatrix} = \begin{bmatrix}cos\theta & sin\theta & 0 \cr -sin\theta & cos\theta & 0 \cr 0 & 0 & 1\end{bmatrix} \begin{bmatrix}I_\alpha \cr I_\beta \cr I_\gamma \end{bmatrix}\\]
Where \\( \theta\\) is the electrical angle of the rotor; the mechanical angle divided by pole pairs and where the bottom and right rows of the matrix are ignored (we assume \\( \gamma\\) is zero).

The backward, or inverse, form of the transform is used on the voltages to create a 2 phase stator reference and then a 3 phase stator reference voltage from the 2 phase rotor reference. These are performed in the function writePWM(_motor) which is run in the fast AND hyperloop to enable voltage injection for HFI.

Inverse Park:
\\[ \begin{bmatrix}V_\alpha \cr V_\beta \cr V_\gamma \end{bmatrix} = \begin{bmatrix}cos\theta & -sin\theta & 0 \cr sin\theta & cos\theta & 0 \cr 0 & 0 & 1\end{bmatrix} \begin{bmatrix}V_d \cr V_q \cr V_0 \end{bmatrix}\\]
Inverse Clarke:
\\[ \begin{bmatrix}V_u \cr V_v \cr V_w \end{bmatrix}=  \begin{bmatrix} 1 & 0 & 1\cr -0.5 & \sqrt{3}/2 & 1\cr 1-.5 & -\sqrt{3}/2 & 1 \end{bmatrix} \begin{bmatrix}V_\alpha \cr V_\beta \cr V_\gamma \end{bmatrix}\\]
MESC uses the full form of the inverse clark where many other implementations skip it and use a SVPWM routine. MESC does this to enable a variety of clamping and over modulation methods, and because it is easier to understand, with no unexplained leaps of faith.
The end result is identical.

\\( sin\theta\\) and \\( cos\theta\\) are calculated from a lookup table 320 elements (=256x1.25) long optionally with interpolation. With interpolation, the maximum error from this is very small; less than the ADC or PWM resolution.

### The Sensorless Observer
The MESC sensorless observer is also known as the MXLEMMING observer and is now the default on the VESC project.
#### What MESC Does
The sensorless observer is very simple. The implementation is unique to MESC and was developed without recourse to appnotes or papers. It is probably not unique in industry, but so far I have not seen it in any other open source or commercial source project.
It works (as most successful observers do) on the basis of flux integration, that is the assumption that for a spinning magnet passing a coil, the voltage is given by:
\\[V = turns x \frac{d\phi}{dt} \\] 
and we observe from watching the motor on a scope that the voltages are sinusoidal.

Therefore, in general, if we ignore the number of turns and make \\(\theta = \omega t\\):
\\[ V = \phi\omega sin(\omega t)\\]
\\[\int V dt = turns \ast \phi +C -> -\phi\cos\theta +C\\] 
We do not need to care for turns, and C varies only dependent on where we start the integration for a sin wave.
The key recognition is that \\( \phi \\) is a constant dependent on the magnets, and therefore the max and min of the resulting integral are symetric and constant.
Since the voltage is sinusoidal, the flux integral will thus also be sinusoidal, with a phase shift of 90 degrees.
(remember to insert pics of sin and integral...)
Further, the addition of noise on the incoming voltage signal is effectively filtered out by this integral since 
\\[ \int cos(n\theta) dt = \frac{cos(n\theta)}{n} (+C) \\] 
and so noise and higher harmonics are greatly reduced.

Within MESC, we choose to carry out this integral in alpha beta frame, so we first remove the effects of resistance and inductance, and then integrate the resulting voltage as:
\\[ V_\alpha = V_{BEMF\alpha} + Ri_\alpha + \frac{Ldi_\alpha}{dt}\\]
\\[ V_\beta = V_{BEMF\beta} + Ri_\beta + \frac{Ldi_\beta}{dt}\\]
where \\(V_\alpha \\) and \\(V_\beta \\) are the electrical voltage output by the inverter and \\(i\alpha\\) and \\(i\beta\\) is the clarke transformed current measured by the ADC.
Thusly, we generate two estimated back EMF voltages which we can integrate to get two flux linkages with a 90 degree phase shift.

\\[ \phi_\alpha = \int V_{BEMF\alpha} dt \\]
\\[ \phi_\beta = \int V_{BEMF\beta} dt \\]

We have to deal with teh +C term in the integral, and also with integration drift which would result in arctangent not working. MESC simply clamps the flux integral at hard limits which can either be fixed or calculated in realtime by the flux linkage observer. 
Since they are shifted by 90 degrees and already filtered by integration, we need only find the arctangent of the two to calculate an estimated angle.
\\[ \theta = arctan(\frac{\phi_\beta}{\phi_\alpha}) + \pi\\]

#### Alternatives MESC chose not to do
Alternative to treating the inductance as a piecewise integral, the Lia term can be lumped. This would remove the need to store previous state information to calculate \\( \frac{di}{dt}\\) and is the method commonly used in literature. 
However, this allows the Lia term to get large compared to the back EMF, and the bounding/elimination of integrational drift is done while the inductance term is still within the BEMF, with probable impact on the result (note the result becomes clearly unstable when \\(Lia>phi\\))
Noteably the Ortega observer as used originally in VESC contains this construction, and relied on a non linear (quadratic) elimination of integration drift and associated instability at high current.

Alternative to the clamping of the flux integrals at their max possible limits a proportional (or PI or non linear) correction factor could be introduced based on the magnitude of the current alpha and beta fluxes. This is similar to the Ortega observer. MESC includes a version of this that can be compiled in with a #define USE_NONLINEAR_OBSERVER_CENTERING but it is advised you do not use this; for experiment only.

Alternatively to the arctangent we could construct a true observer:
\\[ \theta est_{n+1} = \theta est_n + d\theta + k_p*(\theta calc-\theta est)\\]
Where:
\\[ d\theta_{n+1} = d\theta_n + k_i*((\theta est_n + d\theta)-\theta calc_{n+1})\\]
(here we calculate \\(\theta calc \\) through arctangent as above and forward predict/correct our prediction each cycle)

Or: 
\\[ \theta est_{n+1} = \theta est_n + d\theta + k_p*\phi_d\\]
where
\\[ d\theta_{n+1} = d\theta_n + k_i*\phi_d\\] 
(here we use the d axis flux linkage, which we derive from a rotation of the alpha beta flux linkage as an estimate of the error to be corrected)

MESC chooses not to use a true observer, since there is no obvious measurable advantage, there are gains to be "tuned" which can result in instability with a true observer and there are additional calculation steps.
Noteable users of true observers include ST Micro's FOC library which uses a Leunburger observer and Alex Evers' UNIMOC which uses a Kalman filter.
Using a true observer of any kind does not deal with the three most fundamental problems facing sensorless observers: 
* Initially estimating parameters R and L, 
* Changing resistance with temperature and 
* Changing inductance with saturation at high current.

#### The MESC Salient Observer
MESC contains an observer for salient motors, which accounts for the differing d and q inductances. This is not usually required, is not well tested and will not work for outrunner motors since they saturate so heavily.
It relies on the assumption that the salience travels with the dq frame, and can be transformed into the alpha beta frame, then:

\\[ \frac{dLi}{dt} = \frac{Ldi}{dt} + \frac{idL}{dt} \\]

And therefore the above estimates for VBEMF can be modified to account for this changing salience in alpha beta frame.

### The FOC PI
The FOC PI is very simple. It is found in MESCfoc.c in the function MESCFOC(_motor).

It takes the current measurements in dq frame (they were previously collected from the current sensors and transformed by the Clarke and Park transform), calculates an error relative to the PI reference input (mtr[n]->FOC.Idq_req) and applies a change to the output voltage through a proportional and integral gain.
The gains are in units of "Volts/Amp" for the proportional and per second for the integral

The PI is a series PI, that is, the integral gain acts on the output of the proportional gain so:

\\[I_{err} \begin{bmatrix}d \cr q \end{bmatrix} = (I_{req} \begin{bmatrix}I_d \cr I_q \end{bmatrix} - I \begin{bmatrix}d \cr q \end{bmatrix} \times I_{pgain}\\]
and:
\\[I_{int-err} \begin{bmatrix}d \cr q \end{bmatrix} = I_{int-err} \begin{bmatrix}d \cr q \end{bmatrix} + I_{err} \begin{bmatrix}d \cr q \end{bmatrix} \times I_{igain} \times pwmperiod\\]
The output is then calculated as:
\\[\begin{bmatrix}V_d \cr V_q \end{bmatrix} = [I_{int-err} \begin{bmatrix}d \cr q \end{bmatrix} + I_{err} \begin{bmatrix}d \cr q \end{bmatrix}\\]
That's it. Nothing complex about the PI controller.

#### Gains
The trickier thing is how to set the gains, since it is quite possible to create gains that are orders of magnitude wrong, and wrong relative to each other. The target should be that there is response within a few PWM cycles; a few hundred us at most.

The gains can be calculated by setting a desired bandwidth ( mtr[n]->FOC.Current_bandwidth). The proportional gain is simply bandwidth*inductance and the integral gain is the ratio of inductance to resistance.

This is best examined in unit terms; bandwidth is \\(\frac{radians}{second}\\) inductance is \\(\frac{volts\times seconds}{amps}\\) giving pgain in units \\(\frac{volts}{amp}\\).

Likewise, if resistance is \\(\frac{volts}{amps}\\) and inductance is \\(\frac{volts \times seconds}{amps}\\) then Igain works out as \\(Igain = \frac{R}{L} = \frac{1}{seconds}\\).

Making the gains such means that the control loop is critically damped; the fastest reponse possible for a given bandwidth without overshoot.

### The Field Weakening
MESC runs two different field weakening methods, V1 is a dumb ramp of -Id between a starting duty and max duty. V2 is a closed loop field weakening system, where Id is added in response to reaching the duty threshold.
Both methods are run within the MESCfoc() function.
Both methods account for the total current allowed by reducing the Iq request in the slowloop as 
\\[ I_{qmax} = \sqrt{I_{max}^2-I_{FW}^2}\\]
Therefore, if you set lots of field weakening, the total motor current is conserved and you will not burn the coils (any more than you would otherwise, and you may still burn the core with greater iron losses)
#### The dumb ramp
Not much to say... it just ramps up the d-axis current with increasing duty. This is not always stable, if the ramp is too aggressive, it can cause reduction in duty which then causes field weakening to ramp down the next cycle and... oscillation.

#### The closed loop
A much smarter system implemented as a closed loop PI controller (proportional gain set to zero). As long as the bandwidth on the control is much lower than the current loop, it will be stable. That's not to say all motors and controllers are stable under field weakening.
If duty is greater/equal to than max duty:
\\[ I_{FW} = I_{FW} + 0.01 \times I_{FW-max}\\] 
else:
\\[ I_{FW} = I_{FW} - 0.01 \times I_{FW-max}\\]

Experiments with: 
\\[I_{FW} = K_p\times(duty-duty_{max}) + I_{FWint} \\] 
with:
\\[ I_{FWint} = I_{FWint} + K_i\times(duty-duty_{max})\\]
showed no improvement to stability or performance, and additional complication with gain tuning. It may be ressurected at a later date.

### The Circle Limiter
The circle limiter is a not so understood but absolutely critical part of the FOC controller. It's purpose is to ensure that the voltages sent to the inverter do not exceed what the inverter can actually produce, while keeping the PI controller happy. 

It must limit both the overall signal AND limit the PI integral, and do so in the 2D dq frame.

First, we must calculate the maximum length of the voltage vector, this is given by equating the hypotenuse of a 120 degree triangle to the bus voltage, then Vmagmax is the length of one of the shorter sides.

[insert picture of SV triangle]
\\[ V_{magmax} = \frac{1}{\sqrt{3}}V_{bus}*MAXMODULATION\\] 
Max modulation is a limit made to ensure there is always some PWM low time to ensure the bootstrap capacitors can recharge. If using isolated supplied, or overmodulation, this may not be required and you can set 1.0 or even 1.1 (beyond that it becomes quite unstable).

Typically, MAX_MODULATION is set to 0.95.

The identity:
\\[\sqrt(V_d^2+V_q^2)<=V_{magmax}\\]
should always be true.

To achieve this, MESC has two options:
#### Simple linear circle limiter
if: 
\\[\sqrt(V_d^2+V_q^2)>V_{magmax}\\] 
We can simply divide Vd and Vq by this value, and return them linearly to within the range of the circle.

Of course, we also need to ensure that the integral is not winding up in the background, and so we apply the rule:
if:
\\[|int_{Vderr}|>|V_d|\\]
\\[int_{Vderr}=V_d\\]
And similarly for Vq.
This is the default mode for ST Motor control library; it is the most obvious and easiest to implement solution.

#### Vd preferencing circle limiter

There may be reasons to prefer Vd to Vq. One such reason is that when we apply the field weakening, we need to ensure there is sufficient voltage available to generate the d axis current. Since the field weakening current is typically set lower than the torque current, a linear implementation of the circle limiter will result in reduced d axis current with increasing throttle.

To fix this, we give preference to the d-axis, but only up to a point, since with a fixed amount of available voltage, we find that forever favouring Vd results in a rapid drop in Vq as we pass Vd=Vq.

This results in collapse of the q axis current and its ability to control itself.

Presently MESC uses 60degrees as the threshold, i.e. 
\\[V_{dmax} = V_{magmax}*sin(60)\\] 
and therefore implicitly (not calculated!)
\\[V_{qmax} = V_{magmax}*cos(60)\\]
Hard coded as 0.866 (and implicitly 0.5). 

We firstly reduce the d axis voltage as:
\\[ if(V_d > V_{magmax}*0.866) \\]
\\[V_d = V_{magmax}*0.866 \\]
and then apply the circle limiting rule:
\\[V_{qmax} = \sqrt{V_{magmax}^2-V_d^2}\\]
And apply the q limit:
\\[if(V_q > V_{qmax}) \\]
\\[V_q = V_{qmax}\\]
And thusly we have an overall voltage magnitude that does not exceed the max voltage circle.

We apply the same rule to the integral terms as in the linear case to avoid windup.

ST make a variant of this for their motor control library as a selectable option. It does not include the limitation on the Vd proportion, but is otherwise very similar.

#### Things MESC does NOT do
There are blog posts from TI showing improved limiters that avoid current overshoot. They rely on reducing the integral term by the proportional term, with no regard for the ki. This is able to cause large jumps in the integral term with a gain of kp*input noise.

VESC implements the circle limiter this way, and it works, but it is possible to induce fluctations at high modulation. I think TI might have missed out the ki term from the integral limit feedback in their blog.

It is possible to implement this as a PID controller which provides a softer cutoff as the circle limit is approached. So far, I have not seen any reason to attempt this.

It is also possible to set the limits hard by precalculating the required Vd and Vq given the inductance and expected max velocity. This was the original MESC concept and worked well, but always resulted in not quite reaching max modulation, and therefore a few % drop in speed.

There are various non linear possibilities where exceeding the circle limit might have a quadratic rollback, or the the Vd and Vq might be bounded by a higher order flat bottomed polynomial. One day, if the present implementation is found limiting, MESC might adopt some other technique... until then, the method is conservative truncation.


### Tracking
Tracking is simple and relies on phase voltage meaurement.

The phase voltages are measured when PWM is disabled, and these measurements are Clarke transformed to get ab frame voltages, then Park transformed to get dq frame voltages.

The ab frame voltages are used in the flux observer to track the angle and the dq frame voltages are used to preload the integral component of the current controller PI so that when it restarts PWM there is no discontinuity.

The PWM is re-disabled every time the tracking loop is run to ensure that entering tracking from any state is safe (except in the case of heavy field weakenning when the slow loop does not move to tracking until the field weakenning current has dropped). Nothing more to it.

### The Hall start

Hall startup can be set using:

define USE_HALL_START

define HALL_VOLTAGE_THRESHOLD x.y

efine HALL_IIRN 0.0x

The hall start preloads the observer flux integrals with fluxes as monitored during the TRACKING state using a low pass infinite impulse response filter. In the tracking state, there is no current flow and therefore the estimate of the fluxes is unaffected by resistance and inductance; they are, broadly correct.

When the observer is subsequently called, the fluxes are biased towards the average value during that hall state, and thus the angle is strongly biased towards this. The flux integration continues to be carried out during this state, and so when the hall start is switched (at a defined voltage level) the sensorless observer is already running and accurately tracking.

The only parameter to be set is the IIR filter constant, which is by default 0.02 or 50PWM cycles, roughly 60eHz at 20kHz PWM. A faster motor will need a larger value (fewer PWM cycles). Less accurately set resistance or less accurate current measurements might require that you reduce the HALL_IIRN value.

## The Hyperloop

### HFI

#### HFI D

#### HFI45

### PLL and Speed estimation

### Interpolation


## Slow control loop

### Input collection

### Duty cycle control

### The Speed Controller

### Temperature limiting

### MTPA

### Field weakening q-current rollback

### Power limiting





## SimpleComs loop

## RTOS and terminal

