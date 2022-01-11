SimpleEQ is a plugin based off of a youtube course by MatKat Music. 
The differences between this project and the original are mostly in code style, with a few improvements to organization and coding practices thrown in.

[Original Youtube Video](https://youtu.be/i_Iq4_Kd7Rc)

[Original Github Repo](https://github.com/matkatmusic/SimpleEQ)

<img title="Plugin UI" src="/docs/SimpleEQ.PNG">

This plugin is a simple stereo 3-band EQ that features a low pass & high pass filter with 4 different slopes, a peak filter that can either add or cut frequencies with a quality setting, and a response curve and live spectrum analyzer for both left and right channels.

The low pass and high pass filters each range from 20Hz to 20KHz but default to the limits. These filters can be set to 12, 24, 36, or 48 db/Oct cut. The peak filter filter can also have it's center set to anywhere between 20Hz and 20KHz, but defaults to 750Hz. The gain can be pushed up to 24dB, or cut to -24dB.

The response curve accurately shows the changes each of these filters is making with the supplied graph, where the right hand side of the graph shows the dB boost or cut. The left hand side of the graph shows the overall audio level at each frequency, and the blue (left channel) and yellow (right channel) lines show the spectrum analysis for each channel.

All of these components of the plugin can be disabled with their corresponding "power" buttons, and the sliders will be grayed out and won't move when this is done. The spectrum analysis can be turned off by clicking the green button at the top left of the plugin that has the wavy icon.