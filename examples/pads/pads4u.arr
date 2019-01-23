; specify the cell library with the pads
celllibrary pads4u.txt

; create a cell called "padframe"
facet padframe

; place cell "tool-PadFrame" in the center (it is the "core" cell)
core tool-PadFrame

; set the alignment of the pads (specifying input and output port names)
align PAD_in{lay}          dvddL dvddR
align PAD_out{lay}         dvddL dvddR
align PAD_io{lay}          dvddL dvddR
align PAD_vdd{lay}         dvddL dvddR
align PAD_gnd{lay}         dvddL dvddR
align PAD_dvdd{lay}        dvddL dvddR
align PAD_dgnd{lay}        dvddL dvddR
align PAD_flwout{lay}      dvddL dvddR
align PAD_corner{lay}      dvddL dvddR
align PAD_halfSpacer{lay}  dvddL dvddR
align PAD_spacer{lay}      dvddL dvddR
align PAD_raw{lay}         dvddL dvddR

; place the top edge of pads, starting with upper-left
place PAD_corner{lay}
place PAD_gnd{lay} gnd_in=gnd
place PAD_vdd{lay} m1m2=vdd

; place the right edge of pads, going down
rotate c
place PAD_corner{lay}
place PAD_in{lay} out=pulse export in=probePulse
place PAD_spacer{lay}

; place the bottom edge of pads, going left
rotate c
place PAD_corner{lay}
place PAD_out{lay} in=out1 export outbar=probeOut1
place PAD_out{lay} in=out2 export outbar=probeOut2

; place the left edge of pads, going up
rotate c
place PAD_corner{lay}
place PAD_in{lay} out=in1 export in=probeIn1
place PAD_in{lay} out=in2 export in=probeIn2
