# Wow Another Audio Controller
## Deej V2 

This project was inspired by [Omri Harel](https://github.com/omriharel/deej) all credit where credit is do!

- List of materials and stuff
    - This one or similar https://www.aliexpress.us/item/3256805741941228.html
    - Arduino Pro Micro
    - Pot knobbies 10K Ohm
    - Some M3 bolts cause they're tastey
    - 3d printer tbh you could use cardboard it's just cuttin holes in a box
- Things I might eventually do (Probably not)
    - Change it to MISO rather than Serial because serial leads to all sorts of issues with communication between the ESP and the pro Micro some times it leads to crashing of the EXE but I think I've debugged that B)
    - Make the STL files prettier but it works RN and I ran out of the pink filament I used for the Fairly Odd Parents theme
    - Setup the Check Headset thing you see in the code, I might actually do this one because I have a magnet in my headset and a halleffect sensor in my headset holder and wires already hooked up just need to write the code to do it [This Guy](https://github.com/Belphemur/AudioEndPointLibrary/blob/master/DefSound/PolicyConfig.h) did the hard part

# Construction
![Constructed Picture](Imma add it in a bit)
- Print the STLS
- Program the Arudinos
- Solder it up
- Screw it all together
- Slather on some hot glue like you're one of those youtube DIY-ers or edit the STLS and make it prettier
- Plug it in

You can snake the cable in thru the right hole might need to make it bigger or use the left one.

# Soldering Circuit Stuff
![Circuit Picture](Imma add it in a bit)

- Connect it like this ^
- Bridge the 5V's on the Pots together
- Same with the grounds
- Add in data cables and then connect it to Analog pins on the Arduino, you can change them on the sketch.
- Connect RX&TX on the Pro Micro to TX&RX on the ESP, make sure it's RX to TX otherwise it won't work, Might swap it out to use MISO later because better but im lazy.
- Connect 5v and GND to the ESP, all of these can be done thru the connector by the Micro USB in.
- If you wanna change the code on the ESP at all after soldering it make sure you add a switch in on the TX line from the Pro Micro to the RX on the ESP

> [!WARNING]
> If you're a dummy like me  you can do this hardware mod on your ESP, personally I don't notice a difference but idk doesn't hurt to add it ¯\\_(ツ)_/¯
[From Here](https://esp3d.io/esp3d-tft/v1.x/hardware/sunton-35-3248/index.html)
![Don't do this](https://esp3d.io/esp3d-tft/v1.x/hardware/sunton-35-3248/gt911-int-after-mod.jpg?width=400px)

# Config File Stuff
Idk it's pretty self explanatory from the pushed one
- Slider_Mapping: EXE Names for the processes, Master(Device Audio), or unmapped(Everything that isn't listed as an EXE)
- Setting: Comport is the Comport, Inverse is if your pot is Max output 1 min 1024, buttons is if you have/want to use an ESP display screen thing
- Buttons: Button Names on the ESP you got 12, I couldn't figure out 12 binds but im dumb?
- Button_Mapping: What each button does, I have 8 calculator buttons now 👍
