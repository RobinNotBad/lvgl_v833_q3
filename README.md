## Dendro
这是一个简易的，为奇葩 Linux 嵌入式设备提供多媒体支持的 LVGL 程序。  
* 本分支使用动态链接的 LVGL ，下方有说明  

实现了以下功能：
* 模仿安卓，自制支持各类回调和按键事件的页面管理器
* 基于 lv_100ask 的文件管理器、计算器
* 基于 ffmpeg 的图片查看器、音视频播放器
* 基于 libtimidity-soundfont 分支的 MIDI 播放器
* 小游戏 Flappy Bird

## 运行平台
**全志 V833，Tina Linux，musl-libc**

## 怎么编译源代码
* 建议使用**linux系统**
* 拉取源代码  
* 找一个适合设备系统的交叉编译器  
> 例如 arm-openwrt-linux-muslgnueabi-gcc  
* 将项目文件中 build.sh 中的文件路径改为你的编译器路径  
* 运行 ./build.sh 等待编译完成，输出的可执行文件为 dendro_run  
* res 文件夹里存放了程序所需要的字体和图片资源等  

需要注意：lib 文件夹里的 .so 库文件大多由未改动的源码预编译而来，少量是从系统中直接拿取，目标系统为armv7 musleabi hf。  
你可以用自己的编译器重新编译这些库，从而适配不同的系统环境。  
ffmpeg 需要链接 libz 以实现 png 解码支持。  
对于 liblvgl ，请编译： https://github.com/RobinNotBad/lv_shared_linux 并将 liblvgl.so 放在 lib 文件夹里（已提前预编译了一个）  

## 其他
* 字体使用了：阿里巴巴普惠体 Medium、FontAwesome 5 Free Solid，为了在lvgl中正常使用图标，对这两个字体进行了合并（已放在 res 文件夹里）。若要换用自己的字体，可以使用FontForge软件，将FontAwesome中 #61440 之后的所有图标复制到现有字体中，再批量进行大小缩放和位移。

## 碎碎念
> 其实这项目简直就是通过调库堆起来的，在此给所有这些开源库的开发者磕头了，我磕磕磕磕磕！  
> 而且我自己也知道，这代码可能写得很烂，能跑就行……  
> 但是其中包含了不少资料匮乏、自己摸索出的比较好的解决方案。比如图片查看器的自适应大小显示，翻遍搜索引擎甚至问 AI 都没问出来这样的方法，但实现起来超级简单。MIDI 播放器也是走了不少弯路才找到 libtimidity 这个既小又实用的东西。  
> LVGL 本身就是针对单片机和低性能设备，很少有人会把这些多媒体功能整合在一起，用 Linux 系统的更是少之又少。  
> 做这个项目是因为某款 Linux 词典笔的性能极差，原机程序功能极少，放个 mp3 都不行。因为原机程序就是用 LVGL 开发的，所以我也为其适配了 LVGL。  
> 总之，Dendro 现在是一个比较完整的，能为奇葩 Linux 设备提供一点点可玩性的小程序了。  
> 后续有什么规划？也许会引入 lua 脚本支持，这样 Dendro 就可以成为这类设备的一套通用解决方案了。  

## 直接引用的第三方开源项目

> lv_port_linux: https://github.com/lvgl/lv_port_linux/tree/release/v8.4  
> ffmpeg: https://github.com/FFmpeg/FFmpeg  
> freetype: https://github.com/freetype/freetype/releases/tag/VER-2-13-2  
> libtimidity: https://sourceforge.net/p/libtimidity/libtimidity/ci/soundfont  
> lv_lib_100ask: https://github.com/100askTeam/lv_lib_100ask/tree/release/v8.x  
> libssl: https://github.com/openssl/openssl  
> libcurl: https://github.com/curl/curl  
> cJSON: https://github.com/DaveGamble/cJSON
