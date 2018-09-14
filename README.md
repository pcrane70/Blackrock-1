![Alt text](resources/blackrock-title.png?raw=true "Blackrock")

[About](#about)  
[Current Features](#current-features)  
[Upcoming Features](#upcoming-features)  
[Installing](#installing)  
[Contributing](#contributing)  
[Copyright & Licensing](#copyright--licensing)

[Contact](#contact)

## About

**Blackrock** is currently a dungeon exploration rouguelike heavily inspired by the great **Nethack**, nut it is intended to become a much complex game featuring multiplayer and some bigger maps to explore with your friends.

**Blackrock** first started as a hobby program just to test my skills working with C, but as time went one, it became one of my biggest projects and I have to say that I have had a lot of fun working on it and also I have learned a lot.

This game is also intended to serve as a much more complex tutorial for anyone that would like to sharpen his or her skills in C. It includes many topics that I have had a hard time finding out how to implement them in C such as a database using Sqlite or some object pooling and an ECS. In the future it will feature a full wiki for any topic related to the game and its development. For now the code is heavily commented and I will try to keep that way trough its development. If you have any question please contact me.

## Current Features

v 1.0  
  
- ECS  (Entity Component System)
- Object Pooling  
- Simple dungeon generation  
- Ascii characters  
- Simple random enemy spawner  
- Loot and currency  
- Weapons and armor  
- Some items  
- Shops  
- Simple inventory system  
- Simple UI  
- Only one class -> warrior  
- Only one race -> human  
- Support for Linux

## Upcoming Features

v 2.0  
  
 - Better UI
- New player profile    
- New player level system  
- Bosses!  
- Better item system  
- Crafting system  and resources gathering 
- More classes:
- Under the hood performance optimizations like multi-threading  
- Adding sounds!  
- Adding new and better graphics  
- Fully support for mac  
  
v 3.0  
  
- New player profile creation  
- New game mechanics  
- Bigger open world  
- Multiplayer
- Achievements  
- Global Leader boards
- More dungeons!  
- More bosses!  
- More items!

## Installing

### Linux

_Build it yourself_ -> You need to install the following dependencies first:

- Sdl 2.0 - libsdl2-dev
- Sqlite3 - libsqlite3-dev

If you are using Ubuntu, you can run the following commands: 

``` sudo apt-get install libsdl2-dev ```
``` sudo apt-get install sqlite3 libsqlite3-dev ```

Note: You also need to have installed:

- Make

_Compiling_ -> Just run make in the project folder.

_Running_ ->  Use make run.

**_Just give me the exe_** -> If you just want to download the executable, check the releases tab in the top of this page and select the one you want.

## Contributing

Anyone can make a pull request and I will try to check it, and if it seems fine with the flow of the game, I will include it on the project.

## Copyright & Licensing

Pending...

## Contact

You can contact me for whatever you need at erick_salas@ermiry.com.