# GG飛車
## 介紹
此為成圖技術與應用課程的期末專案<br>
使用PhysX物理引擎模擬並以OpenGL繪製<br>
遊戲模型取自手機遊戲《極速領域》<br>
專案架構：
```
├─FinalProjectWithGLUT.sln
└─FinalProjectWithGLUT
    │    game.cpp / game.h         遊戲控制及繪製、物理模擬
    │    main.cpp                  視窗控制及遊戲主選單
    │    printer.cpp / printer.h   用來在畫面上印出ASCII編碼內的字母及符號
    │    setting                   遊戲設定檔
    ├─include
    ├─lib
    ├─fbx                          FBX Loader物件，改自FBX SDK內的Sample ViewScene
    ├─model                        賽車、賽道模型fbx檔、紋理tga檔，賽道_terrian.obj用來創建賽道Rigidbody
    ├─physx                        PhysX官方提供的示範程式專案Snippet的部分程式，用來簡化創建車輛物件過程，
    │                              僅修改部分數值
    └─picture                      Printer用到的字母紋理、遊戲選單及設定背景圖片
```
## 操作
```
WASD    移動
L       手煞車
R       恢復至上一個儲存點(被地形卡住時使用)
JK      作弊功能，可無視地形直接向前後快速移動
```
## 下載
成品：https://drive.google.com/file/d/1B6I6g-9iLVcwlU7talHCdQHAf4Am6HRU/view?usp=sharing<br>
函式庫：https://drive.google.com/file/d/1SisYMnydmqgfYiP_jfjejtqD9e554ojA/view?usp=sharing<br>
## 遊戲畫面<br>
![Image](https://github.com/lksj51790q/gg-speed/blob/main/GGSpeedDemo.gif)<br>
