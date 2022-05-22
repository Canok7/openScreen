# openScreen
投屏
个人业余时间写的一些相关demo。
当前正在逐渐增加子模块，前期已单个独立的demoapp 方式，待完成后进行串联整合
20210803：子目录下clientAndroid/live555
        使用live555+mediacode(ndk） 拉流，解码渲染 rtp-udp h264
20210811:新增 clientAndroid/aacplayer
        使用mediacode解码aac文件，c反过来调用java的AudioTrack 播放（或者可以使用opensl，android AudioTrack在ndk层未开放）
![Screenshot_2022-05-22-15-12-23-291_com example live555](https://user-images.githubusercontent.com/41246028/169683778-25e1c009-e35e-4c75-9fd5-ab8fc8b971a3.png)
