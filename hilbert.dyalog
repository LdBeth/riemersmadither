∇idx←cgil(x y);gilbert
 ⎕NA'gilbert.dylib|gilbert >I4[] I4 I4'
 idx←⎕IO+gilbert(x×y)x y
∇

∇out←priem img;isize;line;idx
 ;len;size;w;a;b;i;⎕IO
 ⎕IO←1
 isize←⍴img
 idx←cgil isize
 line←(,img)[idx]
 size←⌈0.5*⍨len←≢line
 line←(size size⍴line÷255),size 16⍴0
 w←162÷⍨(⌽⍳16)×*15÷⍨⍟15
 out←size size⍴0
 i←1
loop:
 a←line[;i]
 b←0.5≥a
 out[;i]+←b
 line[;i+⍳16]+←w×⍤2 1⊢⍪b-a
 i+←1
 →(i≤size)/loop
 out←isize⍴(,out)[⍋idx]
∇

∇img←readpnm file;data;x;y;type;⎕IO
 ⎕IO←1
 data←83 ¯1 ⎕MAP file
 type←((3 2⍴'P4P5P6')∧.=⎕UCS 2↑data)/pbm pgm ppm
 →(0=≢type)⍴err
 data←3↓data
 x y←⌽⍎⎕UCS ¯1↓(⊢↑⍨⍳∘10)data
 →type
pbm:
 data←,⍉(8⍴2)⊤256|(⊢↓⍨⍳∘10)data
 img←y(↑⍤1)(x,8×⌈8÷⍨y)⍴data
 →0
pgm:
 img←x y⍴256|(⊢↓⍨⍳∘10)⍣2⊢data
 →0
ppm:
 img←(1⌽⍳3)⍉x y 3⍴256|(⊢↓⍨⍳∘10)⍣2⊢data
 →0
err:'Unknown file magic'
∇

∇img writepnm(spec file);hdr;dta;x;y;tie
 →((3 2⍴'P4P5P6')∧.=spec)/pbm pgm ppm
 →err
pbm:
 hdr←10,⍨⎕UCS(⍕x y←⌽⍴img)
 dta←,⍉(8×⌈8÷⍨x)↑⍉img
 dta←2⊥⍉((8÷⍨≢dta),8)⍴dta
 →fin
pgm:
 hdr←10 50 53 53 10,⍨⎕UCS(⍕⌽⍴img)
 dta←,img
 →fin
ppm:
 hdr←10 50 53 53 10,⍨⎕UCS(⍕x y←⌽1↓⍴img)
 dta←,y(3×x)⍴(¯1⌽⍳3)⍉img
fin:
 tie←file(⎕NCREATE⍠'IfExists' 'Error')0
 :Trap 0
     tie ⎕ARBOUT(⎕UCS spec),10,hdr,dta
 :Else
     'Error occured when write file'
 :EndTrap
 ⎕NUNTIE tie
 →0
err:'Unknown file kind'
∇