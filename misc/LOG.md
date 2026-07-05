### [commit:e52f310] fix: Refactored and fixed legal move generator. 
*Too little games to actually find out whether that results in any strength improvement.*
```
Score of leaf_legal_gen_final vs leaf_legal_gen: 1777 - 1801 - 1922  [0.498] 5500
...      leaf_legal_gen_final playing White: 469 - 1337 - 944  [0.342] 2750
...      leaf_legal_gen_final playing Black: 1308 - 464 - 978  [0.653] 2750
...      White vs Black: 933 - 2645 - 1922  [0.344] 5500
Elo difference: -1.5 +/- 7.4, LOS: 34.4 %, DrawRatio: 34.9 %
SPRT: llr 0 (0.0%), lbound -inf, ubound inf
```

### [commit:98b169e] feature: Legal move generator
```
Score of leaf_legal_gen vs leaf_obv_base: 1979 - 1971 - 1550  [0.501] 5500
...      leaf_legal_gen playing White: 1483 - 485 - 782  [0.681] 2750
...      leaf_legal_gen playing Black: 496 - 1486 - 768  [0.320] 2750
...      White vs Black: 2969 - 981 - 1550  [0.681] 5500
Elo difference: 0.5 +/- 7.8, LOS: 55.1 %, DrawRatio: 28.2 %
SPRT: llr 0 (0.0%), lbound -inf, ubound inf
```

### [commit:2dd1cb8] feature: Obvious (single) move in Root
```
Score of leaf_obv vs leaf_master: 1788 - 1717 - 1995  [0.506] 5500
...      leaf_obv playing White: 1293 - 486 - 971  [0.647] 2750
...      leaf_obv playing Black: 495 - 1231 - 1024  [0.366] 2750
...      White vs Black: 2524 - 981 - 1995  [0.640] 5500
Elo difference: 4.5 +/- 7.3, LOS: 88.5 %, DrawRatio: 36.3 %
SPRT: llr 0 (0.0%), lbound -inf, ubound inf
```

### [commit:22691b8] feature: Using _pext_u64 for slider attacks
```
Score of leaf_bmi2 vs leaf_native: 1581 - 1494 - 925  [0.511] 4000
...      leaf_bmi2 playing White: 244 - 1276 - 480  [0.242] 2000
...      leaf_bmi2 playing Black: 1337 - 218 - 445  [0.780] 2000
...      White vs Black: 462 - 2613 - 925  [0.231] 4000
Elo difference: 7.6 +/- 9.4, LOS: 94.2 %, DrawRatio: 23.1 %
SPRT: llr 0 (0.0%), lbound -inf, ubound inf
```

### [commit:ba566a3] feature: Tuned Aspiration Window
*SIDE NOTE: Testing performed on 1s+40ms time control with Hash size of 1mb with Syzygy TB enabled. Parameters tuning done via Parallel SPSA tuning under 4s+100ms time control with Hash set to 32mb.*
```
Score of leaf-tuned-asp vs leaf-final-asp: 1625 - 1450 - 1925  [0.517] 5000 
...      leaf-tuned-asp playing White: 939 - 616 - 945  [0.565] 2500 
...      leaf-tuned-asp playing Black: 686 - 834 - 980  [0.470] 2500 
...      White vs Black: 1773 - 1302 - 1925  [0.547] 5000 
Elo difference: 12.2 +/- 7.5, LOS: 99.9 %, DrawRatio: 38.5 %
SPRT: llr 0 (0.0%), lbound -inf, ubound inf
```

### [commit:bc8c254] feature: Aspiration Window first attempt
*SIDE NOTE: Testing performed on 1s+40ms time control with Hash size of 1mb.*
```
Score of leaf-final-asp vs leaf-final-last: 1244 - 1217 - 1539  [0.503] 4000 
...      leaf-final-asp playing White: 701 - 538 - 761  [0.541] 2000 
...      leaf-final-asp playing Black: 543 - 679 - 778  [0.466] 2000 
...      White vs Black: 1380 - 1081 - 1539  [0.537] 4000 
Elo difference: 2.3 +/- 8.4, LOS: 70.7 %, DrawRatio: 38.5 % 
SPRT: llr 0 (0.0%), lbound -inf, ubound inf 
```

### [commit:1d62259] feature: Tuned search parameters for Syzygy probing
```
Score of leaf-syzygy-tb-tune vs leaf-syzygy-tb: 848 - 778 - 1171  [0.513] 2797 
...      leaf-syzygy-tb-tune playing White: 492 - 320 - 587  [0.561] 1399 
...      leaf-syzygy-tb-tune playing Black: 356 - 458 - 584  [0.464] 1398 
...      White vs Black: 950 - 676 - 1171  [0.549] 2797 
Elo difference: 8.7 +/- 9.8, LOS: 95.9 %, DrawRatio: 41.9 % 
SPRT: llr 0 (0.0%), lbound -inf, ubound inf 
```

### [commit:_] feature: Syzygy 3-4-5 man Tablebase
```
Score of leaf-syzygy-tb vs leaf-prev: 464 - 324 - 479  [0.555] 1267 
...      leaf-syzygy-tb playing White: 259 - 137 - 238  [0.596] 634 
...      leaf-syzygy-tb playing Black: 205 - 187 - 241  [0.514] 633 
...      White vs Black: 446 - 342 - 479  [0.541] 1267 
Elo difference: 38.5 +/- 15.1, LOS: 100.0 %, DrawRatio: 37.8 % 
SPRT: llr 0 (0.0%), lbound -inf, ubound inf 
```

### [commit:_] feature: tuned Singular Extension
```
Score of leaf-sing-tune vs leaf-single-draw: 2202 - 1624 - 174  [0.572] 4000 
...      leaf-sing-tune playing White: 1122 - 792 - 86  [0.583] 2000 
...      leaf-sing-tune playing Black: 1080 - 832 - 88  [0.562] 2000 
...      White vs Black: 1954 - 1872 - 174  [0.510] 4000 
Elo difference: 50.6 +/- 10.6, LOS: 100.0 %, DrawRatio: 4.3 % 
SPRT: llr 0 (0.0%), lbound -inf, ubound inf 
```
### [commit:_] feature: single repetition check
```
Score of leaf-single-draw vs leaf-nprobe: 1425 - 1139 - 1275  [0.537] 3839 
...      leaf-single-draw playing White: 822 - 497 - 601  [0.585] 1920 
...      leaf-single-draw playing Black: 603 - 642 - 674  [0.490] 1919 
...      White vs Black: 1464 - 1100 - 1275  [0.547] 3839 
Elo difference: 25.9 +/- 9.0, LOS: 100.0 %, DrawRatio: 33.2 % 
SPRT: llr 0 (0.0%), lbound -inf, ubound inf 
```
### [commit:_] feature: do not TT-cut in PV node
```
Score of leaf-npv-probe vs leaf-lmr-fix: 1263 - 1196 - 1541  [0.508] 4000 
...      leaf-npv-probe playing White: 709 - 535 - 756  [0.543] 2000 
...      leaf-npv-probe playing Black: 554 - 661 - 785  [0.473] 2000 
...      White vs Black: 1370 - 1089 - 1541  [0.535] 4000 
Elo difference: 5.8 +/- 8.4, LOS: 91.2 %, DrawRatio: 38.5 % 
SPRT: llr 0 (0.0%), lbound -inf, ubound inf 
```
### [commit:_] feature: Dynamic Late Reductions & tuning
```
Score of leaf-tune-1 vs leaf-simd-fix: 1166 - 930 - 1904  [0.529] 4000 
...      leaf-tune-1 playing White: 671 - 393 - 936  [0.570] 2000 
...      leaf-tune-1 playing Black: 495 - 537 - 968  [0.489] 2000 
...      White vs Black: 1208 - 888 - 1904  [0.540] 4000 
Elo difference: 20.5 +/- 7.8, LOS: 100.0 %, DrawRatio: 47.6 % 
SPRT: llr 0 (0.0%), lbound -inf, ubound inf 
```
### [commit:_] feature: save PV line in PV nodes and refresh PV line in TT
```
Score of leaf-pv-store vs leaf-contempt: 956 - 891 - 2153  [0.508] 4000 
...      leaf-pv-store playing White: 547 - 388 - 1065  [0.540] 2000 
...      leaf-pv-store playing Black: 409 - 503 - 1088  [0.476] 2000 
...      White vs Black: 1050 - 797 - 2153  [0.532] 4000 
Elo difference: 5.6 +/- 7.3, LOS: 93.5 %, DrawRatio: 53.8 % 
SPRT: llr 0 (0.0%), lbound -inf, ubound inf 
```
### [commit:_] feature: contempt factor
```
Score of leaf-contempt vs leaf-tune-1: 149 - 116 - 238  [0.533] 503 
...      leaf-contempt playing White: 85 - 52 - 115  [0.565] 252 
...      leaf-contempt playing Black: 64 - 64 - 123  [0.500] 251 
...      White vs Black: 149 - 116 - 238  [0.533] 503 
Elo difference: 22.8 +/- 22.0, LOS: 97.9 %, DrawRatio: 47.3 % 
SPRT: llr 0 (0.0%), lbound -inf, ubound inf 
```
### [commit:_] feature: move ordering and time man parameters tuning
```
Score of leaf-tune-1 vs leaf-time: 411 - 365 - 795  [0.515] 1571 
...      leaf-tune-1 playing White: 234 - 159 - 392  [0.548] 785 
...      leaf-tune-1 playing Black: 177 - 206 - 403  [0.482] 786 
...      White vs Black: 440 - 336 - 795  [0.533] 1571 
Elo difference: 10.2 +/- 12.1, LOS: 95.1 %, DrawRatio: 50.6 % 
SPRT: llr 0 (0.0%), lbound -inf, ubound inf 
```
### [commit:_] feature: time soft limit inside ID framework
```
Score of leaf-time-soft vs leaf-tune: 256 - 237 - 551  [0.509] 1044 
...      leaf-time-soft playing White: 154 - 107 - 261  [0.545] 522 
...      leaf-time-soft playing Black: 102 - 130 - 290  [0.473] 522 
...      White vs Black: 284 - 209 - 551  [0.536] 1044 
Elo difference: 6.3 +/- 14.5, LOS: 80.4 %, DrawRatio: 52.8 % 
SPRT: llr 0 (0.0%), lbound -inf, ubound inf 
```
### [commit:_] feature: parameter-tuning + TT evaluation correction + dynamic improvement + search heuristic limitation tweaks
*SIDE NOTE: Testing performed on 5s+0.1s time control. Parameters tuning done via Parallel SPSA tuning under 60s+600ms time control.*

```
Score of leaf-tune vs leaf-tte-1: 490 - 67 - 232  [0.768] 789 
...      leaf-tune playing White: 255 - 31 - 109  [0.784] 395 
...      leaf-tune playing Black: 235 - 36 - 123  [0.753] 394 
...      White vs Black: 291 - 266 - 232  [0.516] 789 
Elo difference: 208.0 +/- 22.0, LOS: 100.0 %, DrawRatio: 29.4 % 
SPRT: llr 0 (0.0%), lbound -inf, ubound inf 
```
### [commit:_] feature: little changes in rewriting TT
```
Score of leaf-tte-1 vs leaf-tte: 981 - 950 - 2069  [0.504] 4000  
...      leaf-tte-1 playing White: 533 - 422 - 1045  [0.528] 2000  
...      leaf-tte-1 playing Black: 448 - 528 - 1024  [0.480] 2000  
...      White vs Black: 1061 - 870 - 2069  [0.524] 4000 
Elo difference: 2.7 +/- 7.5, LOS: 76.0 %, DrawRatio: 51.7 % 
SPRT: llr 0 (0.0%), lbound -inf, ubound inf 
```
### [commit:_] feature: introduced high hash key and eval in TT
*SIDE NOTE: TESTING PERFORMED ON VERY SHORT TIME CONTROLS AND 1MB TT. Also, it is worth mensioning the eval function was (768->128)x2->1 NNUE. It is expected to have more gain when introducing bigger nets.*
```
Score of leaf-tte vs leaf-avx2: 510 - 471 - 1019  [0.510] 2000 
...      leaf-tte playing White: 308 - 187 - 505  [0.560] 1000 
...      leaf-tte playing Black: 202 - 284 - 514  [0.459] 1000 
...      White vs Black: 592 - 389 - 1019  [0.551] 2000 
Elo difference: 6.8 +/- 10.7, LOS: 89.3 %, DrawRatio: 50.9 % 
SPRT: llr 0 (0.0%), lbound -inf, ubound inf 
```
### [commit:_] feature: (768->128)x2->1 NNUE with SIMD vectorization, fixed little bugs in search
```
Score of leaf-avx2-1 vs leaf-nn64: 1624 - 45 - 331  [0.895] 2000 
...      leaf-avx2-1 playing White: 831 - 24 - 145  [0.903] 1000 
...      leaf-avx2-1 playing Black: 793 - 21 - 186  [0.886] 1000 
...      White vs Black: 852 - 817 - 331  [0.509] 2000 
Elo difference: 371.8 +/- 18.6, LOS: 100.0 %, DrawRatio: 16.6 % 
SPRT: llr 0 (0.0%), lbound -inf, ubound inf 
```
### [commit:_] feature: (768->64)x2->1 NNUE 
```
About ~200 Elo gain
```

### [commit:_] feature: IID
```
Score of leaf-iid-dev vs leaf-rfp: 686 - 643 - 1679  [0.507] 3008 
...      leaf-iid-dev playing White: 384 - 276 - 844  [0.536] 1504 
...      leaf-iid-dev playing Black: 302 - 367 - 835  [0.478] 1504 
...      White vs Black: 751 - 578 - 1679  [0.529] 3008 
Elo difference: 5.0 +/- 8.2, LOS: 88.1 %, DrawRatio: 55.8 % 
SPRT: llr 0 (0.0%), lbound -inf, ubound inf 
```
### [commit:_] feature: reverse fut-pruning return value
```
Score of rfp-dev vs rfp: 736 - 660 - 1829  [0.512] 3225 
...      rfp-dev playing White: 392 - 301 - 920  [0.528] 1613 
...      rfp-dev playing Black: 344 - 359 - 909  [0.495] 1612 
...      White vs Black: 751 - 645 - 1829  [0.516] 3225 
Elo difference: 8.2 +/- 7.9, LOS: 97.9 %, DrawRatio: 56.7 % 
SPRT: llr 0 (0.0%), lbound -inf, ubound inf 
```
### [commit:_] feature: late null move pruning
```
Score of rfp-dev vs rfp: 1329 - 1259 - 3286  [0.506] 5874 
...      rfp-dev playing White: 711 - 580 - 1646  [0.522] 2937 
...      rfp-dev playing Black: 618 - 679 - 1640  [0.490] 2937 
...      White vs Black: 1390 - 1198 - 3286  [0.516] 5874 
Elo difference: 4.1 +/- 5.9, LOS: 91.6 %, DrawRatio: 55.9 % 
SPRT: llr 0 (0.0%), lbound -inf, ubound inf 
```

### [commit:_] feature: reverse futility pruning
```
Score of rfp-dev vs ttq: 1739 - 1461 - 4171  [0.519] 7371 
...      rfp-dev playing White: 929 - 663 - 2094  [0.536] 3686 
...      rfp-dev playing Black: 810 - 798 - 2077  [0.502] 3685 
...      White vs Black: 1727 - 1473 - 4171  [0.517] 7371 
Elo difference: 13.1 +/- 5.2, LOS: 100.0 %, DrawRatio: 56.6 % 
SPRT: llr 0 (0.0%), lbound -inf, ubound inf 
```

### [commit:_] feature: razoring
```
Score of razor-prune-dev vs fut-prune: 573 - 351 - 1141  [0.554] 2065 
...      razor-prune-dev playing White: 298 - 162 - 573  [0.566] 1033 
...      razor-prune-dev playing Black: 275 - 189 - 568  [0.542] 1032 
...      White vs Black: 487 - 437 - 1141  [0.512] 2065 
Elo difference: 37.5 +/- 10.0, LOS: 100.0 %, DrawRatio: 55.3 % 
SPRT: llr 0 (0.0%), lbound -inf, ubound inf 
```

### [commit:_] feature: futility pruning
```
Score of fut-prune-dev vs delta-prune: 967 - 740 - 2079  [0.530] 3786 
...      fut-prune-dev playing White: 531 - 321 - 1042  [0.555] 1894 
...      fut-prune-dev playing Black: 436 - 419 - 1037  [0.504] 1892 
...      White vs Black: 950 - 757 - 2079  [0.525] 3786 
Elo difference: 20.9 +/- 7.4, LOS: 100.0 %, DrawRatio: 54.9 % 
SPRT: llr 0 (0.0%), lbound -inf, ubound inf 
```

### [commit:_] feature: simplest delta pruning
```
Score of delta-prune-dev vs tt-0-probe: 571 - 535 - 1396  [0.507] 2502 
...      delta-prune-dev playing White: 300 - 245 - 706  [0.522] 1251 
...      delta-prune-dev playing Black: 271 - 290 - 690  [0.492] 1251 
...      White vs Black: 590 - 516 - 1396  [0.515] 2502 
Elo difference: 5.0 +/- 9.0, LOS: 86.0 %, DrawRatio: 55.8 % 
SPRT: llr 0 (0.0%), lbound -inf, ubound inf 
```

### [commit:_] feature: tt probe at 0 depth
```
Score of tt-probe-0-dev vs history-gravity-fix: 674 - 395 - 1369  [0.557] 2438 
...      tt-probe-0-dev playing White: 368 - 169 - 682  [0.582] 1219 
...      tt-probe-0-dev playing Black: 306 - 226 - 687  [0.533] 1219 
...      White vs Black: 594 - 475 - 1369  [0.524] 2438 
Elo difference: 39.9 +/- 9.1, LOS: 100.0 %, DrawRatio: 56.2 % 
SPRT: llr 0 (0.0%), lbound -inf, ubound inf 
```

### [commit:_] feature: fixed gravity formula
```
Score of gravity-fix-dev vs history-gravity: 720 - 644 - 1697  [0.512] 3061 
...      gravity-fix-dev playing White: 384 - 285 - 862  [0.532] 1531 
...      gravity-fix-dev playing Black: 336 - 359 - 835  [0.492] 1530 
...      White vs Black: 743 - 621 - 1697  [0.520] 3061 
Elo difference: 8.6 +/- 8.2, LOS: 98.0 %, DrawRatio: 55.4 % 
SPRT: llr 0 (0.0%), lbound -inf, ubound inf 
```

### [commit:_] feature: history gravity formula with maluses
```
Score of history-gravity-dev vs 1mb-3tier-tt: 1092 - 917 - 2682  [0.519] 4691 
...      history-gravity-dev playing White: 614 - 405 - 1327  [0.545] 2346 
...      history-gravity-dev playing Black: 478 - 512 - 1355  [0.493] 2345 
...      White vs Black: 1126 - 883 - 2682  [0.526] 4691 
Elo difference: 13.0 +/- 6.5, LOS: 100.0 %, DrawRatio: 57.2 % 
SPRT: llr 0 (0.0%), lbound -inf, ubound inf 
```

### [commit:_] feature: 3-tier bucket + another replacement scheme
*SIDE NOTE: TESTING PERFORMED ON VERY SHORT TIME CONTROLS, RESULTS MAY DIFFER IN LONGER GAMES*
```
Score of 1mb-3tier-tt-dev vs 1mb-2tier-tt: 676 - 620 - 1721  [0.509] 3017 
...      1mb-3tier-tt-dev playing White: 372 - 275 - 862  [0.532] 1509 
...      1mb-3tier-tt-dev playing Black: 304 - 345 - 859  [0.486] 1508 
...      White vs Black: 717 - 579 - 1721  [0.523] 3017 
Elo difference: 6.4 +/- 8.1, LOS: 94.0 %, DrawRatio: 57.0 % 
SPRT: llr 0 (0.0%), lbound -inf, ubound inf 
```

### [commit:_] feature: double-tier bucket and new replacement scheme
*SIDE NOTE: TESTING WERE PERFORMED ON VERY TIGHT TIME CONTROL OF 1 SECOND + 40 MILLISECONDS.*
*SIZE OF TT WAS SET TO ONLY 1MB TO SOMEHOW RECREATE RATES OF OVERWRITING ENTRIES.*
*ENGINE GOT ABOUT x180 LESS TIME TO THINK, IT TRAVERSE ABOUT x180 LESS NODES AND THE 1MB TABLE SIZE*
*IS 1 / 256 OF DEFAULT 256MB TABLE SIZE IN LONGER GAMES*
*STILL, ENGINE PERFORMANCE MAY DIFFER IN LONGER GAMES.*
```
Score of 1mb-buckets-tt-dev vs 1mb-tt: 154 - 101 - 232  [0.554] 487 
...      1mb-buckets-tt-dev playing White: 79 - 49 - 116  [0.561] 244 
...      1mb-buckets-tt-dev playing Black: 75 - 52 - 116  [0.547] 243 
...      White vs Black: 131 - 124 - 232  [0.507] 487 
Elo difference: 38.0 +/- 22.3, LOS: 100.0 %, DrawRatio: 47.6 % 
SPRT: llr 0 (0.0%), lbound -inf, ubound inf 
```

### [commit:_] feature: fixed check extension
```
Score of new-check-ext-dev vs old-check-ext: 1159 - 746 - 2086  [0.552] 3991 
...      new-check-ext-dev playing White: 643 - 333 - 1019  [0.578] 1995 
...      new-check-ext-dev playing Black: 516 - 413 - 1067  [0.526] 1996 
...      White vs Black: 1056 - 849 - 2086  [0.526] 3991 
Elo difference: 36.1 +/- 7.4, LOS: 100.0 %, DrawRatio: 52.3 % 
SPRT: llr 0 (0.0%), lbound -inf, ubound inf 
```

### [commit:_] feature: simplest check extension
```
Score of check-ext-dev vs 46df: 994 - 902 - 2014  [0.512] 3910 
...      check-ext-dev playing White: 547 - 422 - 986  [0.532] 1955 
...      check-ext-dev playing Black: 447 - 480 - 1028  [0.492] 1955 
...      White vs Black: 1027 - 869 - 2014  [0.520] 3910 
Elo difference: 8.2 +/- 7.6, LOS: 98.3 %, DrawRatio: 51.5 % 
```

### [commit:46dfcc6] feature: new Score boundaries
```
Score of newscore vs 48a7: 591 - 340 - 1408  [0.554] 2339 
...      newscore playing White: 334 - 143 - 693  [0.582] 1170 
...      newscore playing Black: 257 - 197 - 715  [0.526] 1169 
...      White vs Black: 531 - 400 - 1408  [0.528] 2339 
Elo difference: 37.4 +/- 8.8, LOS: 100.0 %, DrawRatio: 60.2 % 
SPRT: llr 0 (0.0%), lbound -inf, ubound inf 
```

### [commit:f2b5346] feature: removed PV_NODE checking at PVS 
```
Score of seachscorefix vs d2fa: 402 - 240 - 671  [0.562] 1313 
...      seachscorefix playing White: 246 - 102 - 308  [0.610] 656 
...      seachscorefix playing Black: 156 - 138 - 363  [0.514] 657 
...      White vs Black: 384 - 258 - 671  [0.548] 1313 
Elo difference: 43.1 +/- 13.1, LOS: 100.0 %, DrawRatio: 51.1 % 
SPRT: llr 0 (0.0%), lbound -inf, ubound inf 
```

### [commit:d2fa75f] feature: new PV-Search + Late Move Reduction
```
Score of pvs_lmr vs see_cutoff: 496 - 350 - 534  [0.553] 1380 
...      pvs_lmr playing White: 298 - 136 - 257  [0.617] 691 
...      pvs_lmr playing Black: 198 - 214 - 277  [0.488] 689 
...      White vs Black: 512 - 334 - 534  [0.564] 1380 
Elo difference: 36.9 +/- 14.4, LOS: 100.0 %, DrawRatio: 38.7 % 
SPRT: llr 0 (0.0%), lbound -inf, ubound inf 
```

### [commit:6e0b0d2] feature: SEE pruning in qsearch
```
Score of see_cutoff vs 7576: 1469 - 1375 - 2118  [0.509] 4962 
...      see_cutoff playing White: 866 - 542 - 1073  [0.565] 2481 
...      see_cutoff playing Black: 603 - 833 - 1045  [0.454] 2481 
...      White vs Black: 1699 - 1145 - 2118  [0.556] 4962 
Elo difference: 6.6 +/- 7.3, LOS: 96.1 %, DrawRatio: 42.7 % 
SPRT: llr 0 (0.0%), lbound -inf, ubound inf 
```

### [commit:611e5d1] feature: new MVV-LVA ordering without SEE and dedicated table
```
Score of 611e vs 5df7: 1162 - 1008 - 1575  [0.521] 3745 
...      611e playing White: 695 - 399 - 779  [0.579] 1873 
...      611e playing Black: 467 - 609 - 796  [0.462] 1872 
...      White vs Black: 1304 - 866 - 1575  [0.558] 3745 
Elo difference: 14.3 +/- 8.5, LOS: 100.0 %, DrawRatio: 42.1 % 
SPRT: llr 0 (0.0%), lbound -inf, ubound inf 
```

### [commit:_] feature: smarter see
```
Score of newsee vs oldsee: 262 - 292 - 387  [0.484] 941 
...      newsee playing White: 174 - 101 - 196  [0.577] 471 
...      newsee playing Black: 88 - 191 - 191  [0.390] 470 
...      White vs Black: 365 - 189 - 387  [0.594] 941 
Elo difference: -11.1 +/- 17.0, LOS: 10.1 %, DrawRatio: 41.1 % 
SPRT: llr 0 (0.0%), lbound -inf, ubound inf 
```

### [commit:_] feature: history heuristic without countermove heuristic + select sort + some structural changes in the code
```
Score of history vs nonpv: 555 - 440 - 623  [0.536] 1618 
...      history playing White: 315 - 177 - 317  [0.585] 809 
...      history playing Black: 240 - 263 - 306  [0.486] 809 
...      White vs Black: 578 - 417 - 623  [0.550] 1618 
Elo difference: 24.7 +/- 13.3, LOS: 100.0 %, DrawRatio: 38.5 % 
SPRT: llr 0 (0.0%), lbound -inf, ubound inf 
```

### [commit:_] feature: simple SEE 
```
Score of t2 vs t1: 502 - 480 - 650  [0.507] 1632 
...      t2 playing White: 300 - 192 - 324  [0.566] 816 
...      t2 playing Black: 202 - 288 - 326  [0.447] 816  
...      White vs Black: 588 - 394 - 650  [0.559] 1632 
Elo difference: 4.7 +/- 13.1, LOS: 75.9 %, DrawRatio: 39.8 % 
SPRT: llr 0 (0.0%), lbound -inf, ubound inf 
```

### [commit:_] feature: countermove heuristic
```
Score of t2 vs t1: 1709 - 1766 - 2324  [0.495] 5799 
... t2 playing White: 1017 - 702 - 1181  [0.554] 2900 
... t2 playing Black: 692 - 1064 - 1143  [0.436] 2899 
... White vs Black: 2081 - 1394 - 2324  [0.559] 5799 
Elo difference: -3.4 +/- 6.9, LOS: 16.7 %, DrawRatio: 40.1 % 
SPRT: llr 0 (0.0%), lbound -inf, ubound inf 
```

### [commit:_] feature: Null move pruning with verification search
```
Score of t2 vs t1: 248 - 113 - 198  [0.621] 559 
... t2 playing White: 154 - 43 - 83  [0.698] 280 
... t2 playing Black: 94 - 70 - 115  [0.543] 279 
... White vs Black: 224 - 137 - 198  [0.578] 559 
Elo difference: 85.6 +/- 23.5, LOS: 100.0 %, DrawRatio: 35.4 % 
SPRT: llr 0 (0.0%), lbound -inf, ubound inf 
```

### [commit:ed9f12b] feature: killer moves
```
Score of t2 vs t1: 389 - 298 - 356  [0.544] 1043 
... t2 playing White: 223 - 129 - 170  [0.590] 522 
... t2 playing Black: 166 - 169 - 186  [0.497] 521 
... White vs Black: 392 - 295 - 356  [0.547] 1043 
Elo difference: 30.4 +/- 17.1, LOS: 100.0 %, DrawRatio: 34.1 % 
SPRT: llr 0 (0.0%), lbound -inf, ubound inf 
```

### [commit:_] feature: deeper repetition knowledge
```
Score of t2 vs t1: 684 - 650 - 666  [0.508] 2000 
... t2 playing White: 406 - 251 - 343  [0.578] 1000 
... t2 playing Black: 278 - 399 - 323  [0.440] 1000 
... White vs Black: 805 - 529 - 666  [0.569] 2000 
Elo difference: 5.9 +/- 12.4, LOS: 82.4 %, DrawRatio: 33.3 % 
SPRT: llr 0 (0.0%), lbound -inf, ubound inf 
```

### [commit:e32c61c] feature: added MVV-LVA table
```
Score of t1 vs t2: 420 - 182 - 301  [0.632] 903 
...      t1 playing White: 242 - 64 - 146  [0.697] 452 
...      t1 playing Black: 178 - 118 - 155  [0.567] 451 
...      White vs Black: 360 - 242 - 301  [0.565] 903 
Elo difference: 93.8 +/- 18.8, LOS: 100.0 %, DrawRatio: 33.3 % 
SPRT: llr 0 (0.0%), lbound -inf, ubound inf 
```

### [commit:09453c6] feature: fixed hash move probing 
```
Score of t1 vs t2: 868 - 288 - 518  [0.673] 1674 
...      t1 playing White: 511 - 100 - 226  [0.746] 837 
...      t1 playing Black: 357 - 188 - 292  [0.601] 837 
...      White vs Black: 699 - 457 - 518  [0.572] 1674 
Elo difference: 125.6 +/- 14.3, LOS: 100.0 %, DrawRatio: 30.9 % 
SPRT: llr 0 (0.0%), lbound -inf, ubound inf 
```

### [commit:995bf74] feature: early hash move ordering
```
Score of t2 vs t1: 676 - 623 - 701  [0.513] 2000 
...      t2 playing White: 416 - 237 - 347  [0.590] 1000 
...      t2 playing Black: 260 - 386 - 354  [0.437] 1000 
...      White vs Black: 802 - 497 - 701  [0.576] 2000 
Elo difference: 9.2 +/- 12.3, LOS: 92.9 %, DrawRatio: 35.0 % 
SPRT: llr 0 (0.0%), lbound -inf, ubound inf 
```

### [commit:fc9fa56] feature: storing PV line in TT
```
Score of t1 vs t2: 362 - 259 - 379  [0.551] 1000 
...      t1 playing White: 231 - 96 - 173  [0.635] 500 
...      t1 playing Black: 131 - 163 - 206  [0.468] 500 
...      White vs Black: 394 - 227 - 379  [0.584] 1000 
Elo difference: 35.9 +/- 17.0, LOS: 100.0 %, DrawRatio: 37.9 % 
SPRT: llr 0 (0.0%), lbound -inf, ubound inf 
```

### [commit:bf32e7a] feature: added TT and fixed search bugs
```
Score of t1 vs t2: 684 - 523 - 793  [0.540] 2000 
...      t1 playing White: 402 - 211 - 387  [0.596] 1000 
...      t1 playing Black: 282 - 312 - 406  [0.485] 1000 
...      White vs Black: 714 - 493 - 793  [0.555] 2000 
Elo difference: 28.0 +/- 11.8, LOS: 100.0 %, DrawRatio: 39.6 % 
SPRT: llr 0 (0.0%), lbound -inf, ubound inf
```
