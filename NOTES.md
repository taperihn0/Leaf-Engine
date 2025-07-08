### commit 611e5d1: new MVV-LVA ordering without SEE and dedicated table
Score of 611e vs 5df7: 1162 - 1008 - 1575  [0.521] 3745 <br>
...      611e playing White: 695 - 399 - 779  [0.579] 1873 <br>
...      611e playing Black: 467 - 609 - 796  [0.462] 1872 <br>
...      White vs Black: 1304 - 866 - 1575  [0.558] 3745 <br>
Elo difference: 14.3 +/- 8.5, LOS: 100.0 %, DrawRatio: 42.1 % <br>
SPRT: llr 0 (0.0%), lbound -inf, ubound inf <br>

### commit _: smarter see
Score of newsee vs oldsee: 262 - 292 - 387  [0.484] 941 <br>
...      newsee playing White: 174 - 101 - 196  [0.577] 471 <br>
...      newsee playing Black: 88 - 191 - 191  [0.390] 470 <br>
...      White vs Black: 365 - 189 - 387  [0.594] 941 <br>
Elo difference: -11.1 +/- 17.0, LOS: 10.1 %, DrawRatio: 41.1 % <br>
SPRT: llr 0 (0.0%), lbound -inf, ubound inf <br>

### commit _: history heuristic without countermove heuristic + select sort + some structural changes in the code
Score of history vs nonpv: 555 - 440 - 623  [0.536] 1618 <br>
...      history playing White: 315 - 177 - 317  [0.585] 809 <br>
...      history playing Black: 240 - 263 - 306  [0.486] 809 <br>
...      White vs Black: 578 - 417 - 623  [0.550] 1618 <br>
Elo difference: 24.7 +/- 13.3, LOS: 100.0 %, DrawRatio: 38.5 % <br>
SPRT: llr 0 (0.0%), lbound -inf, ubound inf <br>

### commit _: simple SEE 
Score of t2 vs t1: 502 - 480 - 650  [0.507] 1632 <br>
...      t2 playing White: 300 - 192 - 324  [0.566] 816 <br>
...      t2 playing Black: 202 - 288 - 326  [0.447] 816 <br> 
...      White vs Black: 588 - 394 - 650  [0.559] 1632 <br>
Elo difference: 4.7 +/- 13.1, LOS: 75.9 %, DrawRatio: 39.8 % <br>
SPRT: llr 0 (0.0%), lbound -inf, ubound inf <br>

### commit _: countermove heuristic
Score of t2 vs t1: 1709 - 1766 - 2324  [0.495] 5799 <br>
... t2 playing White: 1017 - 702 - 1181  [0.554] 2900 <br>
... t2 playing Black: 692 - 1064 - 1143  [0.436] 2899 <br>
... White vs Black: 2081 - 1394 - 2324  [0.559] 5799 <br>
Elo difference: -3.4 +/- 6.9, LOS: 16.7 %, DrawRatio: 40.1 % <br>
SPRT: llr 0 (0.0%), lbound -inf, ubound inf <br>

### commit _: null move pruning with verification search
Score of t2 vs t1: 248 - 113 - 198  [0.621] 559 <br>
... t2 playing White: 154 - 43 - 83  [0.698] 280 <br>
... t2 playing Black: 94 - 70 - 115  [0.543] 279 <br>
... White vs Black: 224 - 137 - 198  [0.578] 559 <br>
Elo difference: 85.6 +/- 23.5, LOS: 100.0 %, DrawRatio: 35.4 % <br>
SPRT: llr 0 (0.0%), lbound -inf, ubound inf <br>

### commit ed9f12b: killer moves
Score of t2 vs t1: 389 - 298 - 356  [0.544] 1043 <br>
... t2 playing White: 223 - 129 - 170  [0.590] 522 <br>
... t2 playing Black: 166 - 169 - 186  [0.497] 521 <br>
... White vs Black: 392 - 295 - 356  [0.547] 1043 <br>
Elo difference: 30.4 +/- 17.1, LOS: 100.0 %, DrawRatio: 34.1 % <br>
SPRT: llr 0 (0.0%), lbound -inf, ubound inf <br>

### commit _: deeper repetition knowledge
Score of t2 vs t1: 684 - 650 - 666  [0.508] 2000 <br>
... t2 playing White: 406 - 251 - 343  [0.578] 1000 <br>
... t2 playing Black: 278 - 399 - 323  [0.440] 1000 <br>
... White vs Black: 805 - 529 - 666  [0.569] 2000 <br>
Elo difference: 5.9 +/- 12.4, LOS: 82.4 %, DrawRatio: 33.3 % <br>
SPRT: llr 0 (0.0%), lbound -inf, ubound inf <br>

### commit e32c61c: added MVV-LVA table
Score of t1 vs t2: 420 - 182 - 301  [0.632] 903 <br>
...      t1 playing White: 242 - 64 - 146  [0.697] 452 <br>
...      t1 playing Black: 178 - 118 - 155  [0.567] 451 <br>
...      White vs Black: 360 - 242 - 301  [0.565] 903 <br>
Elo difference: 93.8 +/- 18.8, LOS: 100.0 %, DrawRatio: 33.3 % <br>
SPRT: llr 0 (0.0%), lbound -inf, ubound inf <br>

### commit 09453c6: fixed hash move probing 
Score of t1 vs t2: 868 - 288 - 518  [0.673] 1674 <br>
... t1 playing White: 511 - 100 - 226  [0.746] 837 <br>
... t1 playing Black: 357 - 188 - 292  [0.601] 837 <br>
... White vs Black: 699 - 457 - 518  [0.572] 1674 <br>
Elo difference: 125.6 +/- 14.3, LOS: 100.0 %, DrawRatio: 30.9 % <br>
SPRT: llr 0 (0.0%), lbound -inf, ubound inf <br>

### commit 995bf74: early hash move ordering
Score of t2 vs t1: 676 - 623 - 701  [0.513] 2000 <br>
... t2 playing White: 416 - 237 - 347  [0.590] 1000 <br>
... t2 playing Black: 260 - 386 - 354  [0.437] 1000 <br>
... White vs Black: 802 - 497 - 701  [0.576] 2000 <br>
Elo difference: 9.2 +/- 12.3, LOS: 92.9 %, DrawRatio: 35.0 % <br>
SPRT: llr 0 (0.0%), lbound -inf, ubound inf <br>

### commit fc9fa56: storing PV line in TT <br>
Score of t1 vs t2: 362 - 259 - 379  [0.551] 1000 <br>
... t1 playing White: 231 - 96 - 173  [0.635] 500 <br>
... t1 playing Black: 131 - 163 - 206  [0.468] 500 <br>
... White vs Black: 394 - 227 - 379  [0.584] 1000 <br>
Elo difference: 35.9 +/- 17.0, LOS: 100.0 %, DrawRatio: 37.9 % <br>
SPRT: llr 0 (0.0%), lbound -inf, ubound inf <br>

### commit bf32e7a: added TT and fixed search bugs <br>
Score of t1 vs t2: 684 - 523 - 793  [0.540] 2000 <br>
... t1 playing White: 402 - 211 - 387  [0.596] 1000 <br>
... t1 playing Black: 282 - 312 - 406  [0.485] 1000 <br>
... White vs Black: 714 - 493 - 793  [0.555] 2000 <br>
Elo difference: 28.0 +/- 11.8, LOS: 100.0 %, DrawRatio: 39.6 % <br>
SPRT: llr 0 (0.0%), lbound -inf, ubound inf