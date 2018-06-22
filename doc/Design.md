# Design
The following sections describe the architecture of Scarlett USB Interface cards,
how the ALSA provides the cards features to userspace and how the Scarlett Mixer
application parses and organizes the ALSA elements.

## Focusrite Scarlett USB Audio Interface Cards Architecture
The Focusrite Scarlett USB Audio Interface Cards feature a flexible onboard
channel mixing and routing to setup PCM capture and playback, direct monitoring and separate monitor mixes.

The number of channels depends on the type of the card. For example, the 
Scarlett 6i6 card has 
* 6 mono hardware input channels
* 3 stereo hardware output channels
* 6 mono PCM channels that are sent to the PC
* 12 mono PCM channels that are received from the PC

The 18 input channels (6 hardware and 12 PCM) can be routed to output channels
for direct monitoring or PCM capture and to a multi-stage matrix mixer to 
setup 4 stereo monitor mixes to be routed also to the output channels.

@dot "Scarlett Architecture"
digraph alsa {
    splines="ortho";
    graph [fontname="sans-serif"];
    node [fontname="sans-serif"];
    edge [fontname="sans-serif"];
    
    hwin [shape=cds, label="Hardware Inputs"];
    pcmout [shape=cds, label="ALSA PCM Outputs"];
    matrixmux [shape=invtrapezium, label="Matrix Mux"];
    matrixmixer [shape=box, label="Matrix Mixer\n\n18x8 Gain Stages"];
    mainmux [shape=invtrapezium, width=5, height=0.5, label="Output Mux"];
    mastermux [shape=invtrapezium, label="Master Mux"];
    mastergain [shape=box, label = "Master Gain"];
    hwout [shape=cds, label="Hardware Outputs"];
    capmux [shape=invtrapezium, label="Capture Mux"];
    pcmin [shape=cds, label="ALSA PCM Inputs"];
    
    hwin -> matrixmux [xlabel="6 ch"];
    hwin -> mainmux [xlabel="6 ch"];
    pcmout -> matrixmux [xlabel="12 ch"];
    pcmout -> mainmux [xlabel="12 ch"];
    matrixmux -> matrixmixer [xlabel="18 ch"];
    matrixmixer -> mainmux [xlabel="8 ch"];
    mainmux -> mastermux [xlabel="26 ch"];
    mastermux -> mastergain [xlabel="6 ch"];
    mastergain -> hwout [xlabel="6 ch\n(3 stereo)"];
    mainmux -> capmux [xlabel="26 ch"];
    capmux -> pcmin [xlabel="6 ch"];
}
@enddot

## ALSA
ALSA provides the components of the card architecture as ALSA mixer elements with metadata that
can be queried by ALSA library functions.

The Scarlett 6i6 card for example is represented by ALSA by the following elements and
metadata query results from the `alsa_mixer_selem_*` functions family (X denotes queries that return `true`):
| ALSA Mixer Element     | is_enumerated | is_enum_capture | is_enum_playback | has_playback_switch | has_playback_switch_joined | has_playback_volume | has_playback_volume_joined | is_playback_mono |
|------------------------|---------------|------------------|-----------------|---------------------|----------------------------|---------------------|----------------------------|------------------|
| Master                 |   |   |   | X | X | X | X | X |
| Master 1 (Monitor)     |   |   |   | X |   | X |   |   |
| Master 1L Source       | X |   | X |   |   |   |   |   |
| Master 1R Source       | X |   | X |   |   |   |   |   |
| Master 2 (Headphone)   |   |   |   | X |   | X |   |   |
| Master 2L Source       | X |   | X |   |   |   |   |   |
| Master 2R Source       | X |   | X |   |   |   |   |   |
| Master 3 (SPDIF)       |   |   |   | X |   | X |   |   |
| Master 3L Source       | X |   | X |   |   |   |   |   |
| Master 3R Source       | X |   | X |   |   |   |   |   |
| Input 1 Impedance      | X |   |   |   |   |   |   |   |
| Input 1 Pad            | X |   |   |   |   |   |   |   |
| Input 2 Impedance      | X |   |   |   |   |   |   |   |
| Input 2 Pad            | X |   |   |   |   |   |   |   |
| Input 3 Pad            | X |   |   |   |   |   |   |   |
| Input 4 Pad            | X |   |   |   |   |   |   |   |
| Input Source 01..06    | X | X |   |   |   |   |   |   |
| Martix 01..18 Input    | X |   | X |   |   |   |   |   |
| Martix 01..18 Mix A..H |   |   |   |   |   | X | X | X |
| Sample Clock Source    | X |   |   |   |   |   |   |   |
| Sample Sync Status     | X |   |   |   |   |   |   |   |
| Scarlet 6i6 USB-Sync   | X |   |   |   |   |   |   |   |

## Scarlett Mixer Application
The Scarlett Mixer application organizes the ALSA mixer elements in three different GObject classes:
@dot "Class Diagram"
digraph alsa {
    #rankdir=LR;
    splines="ortho";
    graph [fontname="sans-serif"];
    node [fontname="sans-serif"];
    edge [fontname="sans-serif"];
    
    app [shape=box, label="SmApp"];
    sw [shape=box, label="SmSwitch"];
    src [shape=box, label="SmSource"];
    ch [shape=box, label="SmChannel"];
    
    app -> sw [xlabel="Input Switches", headlabel="*"];
    app -> src [xlabel="Input Sources", headlabel="*"];
    app -> ch [xlabel="Channels", headlabel="*"];
}
@enddot

The application uses metadata of the ALSA mixer elements to determine which GObject the element belongs.
* SmSwitches: The SmSwitch object holds the ALSA mixer elements represeting the various hardware switches of the card.
* SmSource: The SmSource object holds the ALSA mixer elements to select the hardware inputs for the capture channels.
* SmChannel: The SmChannel object holds ALSA mixer elements with a volume property and related source selection elements.

The GObjects provide a `gboolean sm_*_add_mixer_elem(snd_mixer_elem_t *elem)` method adds the ALSA mixer element
to the object instance if certain requirements are met.

### SmSwitch
The SmSwitch GObject accepts the ALSA mixer elements for which the following conditions for the metadata queries are met:
* `is_enumerated` is `true`
* `is_enum_playback` is `false`
* `is_enum_capture` is `false`

For the Scarlett 6i6 card this object contains the Input Impedance and Pad elements, the "Sample Clock Source", the "Sample Sync Status" and the "Scarlet 6i6 USB-Sync" elements.

The switches are further classified in different types based on their element name:
| Switch type     | Element Name Pattern |
|-----------------|----------------------|
| Input Pad       | Input `<X>` Pad        |
| Input Impedance | Input `<X>` Impedance  |
| Clock Source    | Sample Clock Source  |
| Sync Status     | Sample Sync Status   |
| USB Sync        | Scarlet 6i6 USB-Sync |

The GObject for a input switch is implemented in the SmSwitch (@ref sm-switch.h).

### SmSource
The SmSource GObject accepts the ALSA mixer elements for which the following conditions for the metadata queries are met:
* `is_enumerated` is `true`
* `is_enum_playback` is `false`
* `is_enum_capture` is `true`

For the Scarlett 6i6 card this object contains the "Input Source <XY>" elements.

The GObject for a input source is implemented in the SmSource (@ref sm-source.h).

### SmChannel
The SmChannel GObject accepts the ALSA mixer elements for which the following conditions for the metadata queries are met:
* `is_enumerated` is `false`
* `is_enum_playback` is `true`

The channels are classified in different types based on metadata queries and on their element name:
| Channel type | has_playback_switch | has_playback_switch_joined | has_playback_volume | has_playback_volume_joined |
|--------------|---------------------|----------------------------|---------------------|----------------------------|
| Master       | X | X | X | X |
| Output       | X |   | X |   |
| Matrix Mix   |   |   | X | X |
The "Master" channel type is designated to the "Master" element of the card.

The "Output" channel types hold the elements related to the main output channels of the card:
* Master `<X>`
* Master `<X>`L Source
* Master `<X>`R Source

The "Matrix Mix" channel types hold the elements related to the Matrix Mix channels of the card:
* Matrix `<X>` Mix `<Y>`
* Matrix `<X>` Source

The GObject for a channel is implemented in the SmChannel (@ref sm-channel.h).