This is a DIY midi aerophone based on arduino. You can get the demo video on bilibili: 
https://www.bilibili.com/video/BV18K411L79G

This project mainly included a .ino file, which is the arduino program, and also some PC softwares to transfer the midi signal to the sound band, e.g., <strong>Kontakt 5</strong>. In the video we demostrated 《Laputa:Castle in the Sky》 using <strong>Session Horm, Kontakt</strong>.

The procedure of this midi instrument is as follow. First, in the mouthpiece a breath sensor is set up to capture the breath inflow, and turn out with an analog signal as arduino input, which is 0-1023. The signal is then transfered into a 0-127 midi velocity signal. Second, the arduino chip reads the button stats from its digital input pins, determined what note the player is playing currently. At last, according to the breath sensor, the program detects wether the note begins, ends or onplaying(i.e. aftertouch), and constructs thecommand part of the 3 byte midi signal. 

The midi signal is then uploaded to PC through a USB port. Here we use <strong>Hairless midiserial</strong> to read the midi income, and send them to a virtual midi port, which we choose <strong>loopMIDI</strong> as a helper tool. After the virtual midi port is contacted, the sound band is now finally recieving the incoming midi signal.

Here I'm giving the connection of the arduino pins. The instrument mainly includes six note buttons, one sharp button, one flat button,a mothpiece, a screen and a 8va/8vb switch. Also notice that the project is still with bugs and not totally functioning, the biggest problem is that our breath sensor is not sensitive enough to capture the short notes. For now I'm not pretty sure the reason, but I'll try fix it in the next version.

Some functions is not available for now such as keySelect and pitchWheel, the velcity control is also off, you can turn on by change the last parameter of function MIDImessage(), which is "75", into "velocity", which is calculated at the begining of each loop. But I dont guarantee it works well.

Pins:<br>
note buttons----D2~D7<br>
sharp button----D9<br>
flat button-----D10<br>
8va/8vb switch--A1,A2<br>
screen----------SCL,SDA<br>
breath sensor(XGZP6847)---A3<br>
LEDs(WS2812)----D8<br>

有上面的视频和引脚连接说明不用再全文翻译一次了吧...大概就是组成3字节的midi信号输出，通过usb进电脑，用hairless midiserial读取，转发到虚拟midi接口loopmidi，就可以在音色库或宿主软件读到midi信号了。并不知道在这里提供软件下载是否符合版权，后面可能会删掉
