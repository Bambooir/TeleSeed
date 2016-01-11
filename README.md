# [TeleSeed](https://telegram.me/TeleSeed) 
**An advance and powerful Administration bot based on [yagop/telegram-bot](https://github.com/yagop/telegram-bot) licensed under [GNU General public License](https://github.com/SEEDTEAM/TeleSeed/blob/master/LICENSE)**
# Features

* **A powerful Anti spam with custom sensitivity For each group**
* **Multiple Realms(admin groups)**
* **recalcitrant to any kind of spamming(Xy bots,name|photo changers and ...)**
* **Global ban**
* **Broadcast to all groups**
* **Group link**
* **Kick,ban and unban by reply**
* **Groups,ban and global bans list**
* **Logging anything that happens in group !**
* **Invite by username**
* **Group administration in bots private**
* **Only mods,owner and admin can add bots**
* **Arabic lock**
* **And ...**

**Table of Contents** 
- [TeleSeed](#teleseed)
- [Features](#features)
- [Commands](#commands)
	- [Admins commands](#admins-commands)
		- [Realm creation](#realm-creation)
		- [Group creation](#group-creation)
		- [Add or remove realms](#add-or-remove-realms)
		- [Add or remove groups](#add-or-remove-groups)
		- [Leaving](#leaving)
		- [Everything about groups](#everything-about-groups)
		- [Setting description in realm](#setting-description-in-realm)
		- [Setting group name in realm](#setting-group-name-in-realm)
		- [Setting rules in realm](#setting-rules-in-realm)
		- [lock groups name|flood|photo|member in realm](#lock-groups-namefloodphotomember-in-realm)
		- [Unlock groups name|flood|photo|member](#unlock-groups-namefloodphotomember)
	- [Group setting in realm](#group-setting-in-realm)
		- [Add admin](#add-admin)
		- [Remove admin](#remove-admin)
		- [Admins|group list in realm](#adminsgroup-list-in-realm)
		- [Broadcast](#broadcast)
		- [Realm help](#realm-help)
	- [Global ban commands](#global-ban-commands)
		- [Set group owner](#set-group-owner)
		- [Bot stats](#bot-stats)
	- [owners and mods commands](#owners-and-mods-commands)
		- [Hammer](#hammer)
		- [group name|member|arabic|flood|bots lock](#group-namememberarabicfloodbots-lock)
		- [group name|member|arabic|flood|bots unlock](#group-namememberarabicfloodbots-unlock)
		- [Group modlist|rules|about|member clean](#group-modlistrulesaboutmember-clean)
		- [Set groups rules|about](#set-groups-rulesabout)
		- [Setting or changing group owner](#setting-or-changing-group-photo)
		- [Setting or changing group photo](#setting-or-changing-group-photo)
		- [Changing group name](#changing-group-name)
		- [Group link](#group-link)
		- [Promote and demote mods](#promote-and-demote-mods)
		- [Resolve username](#resolve-username)
		- [Flood sensitivity](#flood-sensitivity)
		- [Group rules and about](#group-rules-and-about)
		- [Group settings](#group-settings)
		- [modlist](#modlist)
		- [Help](#help)
		- [Owner](#owner)
		- [Save and get](#save-and-get)
		- [Id](#id)
		- [Group stats](#group-stats)
		- [Member list](#member-list)
		- [Group help](#group-help)
	- [In private commands](#in-private-commands)
		- [Hammer](#hammer)
		- [cleaning](#cleaning)
		- [setting flood sensitivity](#setting-flood-sensitivity)
		- [lock groups member|name](#lock-groups-membername)
			- [unlock groups member|name](#unlock-groups-membername)
		- [Group link](#group-link)
		- [change name|rules|name](#change-namerulesname)
		- [Group log](#group-log)
		- [Join](#join)
- [Installation](#installation)
		- [One command](#one-command)
		- [Realm configuration](#realm-configuration)
- [Support and development](#support-and-development)
- [Special thanks to:](#special-thanks-to)
- [Our team!](#our-team-)


# Commands 
## Admins commands
**Only admins and sudo users can run these commands**
### Group creation
>[!/]creategroup [group name]
>
>>[!/]creategroup SEED
>>>will create a group
>>>
>>>_Only works in realms for admins but, sudo users can use it everywhere_

### Realm creation
>[!/]createrealm [realm name]
>
>>[!/]createrealm SEED
>>>will create a realm
>>>
>>>_Only works in realms for admins but, sudo users can use it everywhere_

###Add or remove realms
>[!/]add realm
>>This command will add that group
>
>[!/]rem realm
>>This command will remove that group


###Add or remove groups
>[!/]add
>>This command will add that group
>
>[!/]rem
>>This command will remove that group

###Leaving
>[!/]leave
>>Bot will leave that group

###Everything about groups
>[!/]all
>>This command will return everything about that group
>
>[!/]all [group_id]
>>_Only onwer,admin and sudo users can use this command


### Setting description in realm
> [!/]setabout [Group_id] [text]
>>[!/]setabout 123456789 about
>>>This command will set [text] as description of [Group_id]


### Setting group name in realm 
> [!/]setname [Group_id] [text]
>>[!/]setname 123456789 SEED
>>>This command will set [text] as name of [Group_id]

### Setting rules in realm
> [!/]setrules [Group_id] [text]
>>[!/]setrules 123456789 rules !
>>>This command will set [text] as rules of [Group_id]


### Lock groups name|flood|photo|member in realm
> [!/]lock [Group_id] [name|flood|photo|member]
>>[!/]lock 123456789 name
>>>This command will lock name|flood|photo|member of [Group_id]


### Unlock groups name|flood|photo|member
> [!/]Unlock [Group_id] [name|flood|photo|member]
>>[!/]Unlock 123456789 name
>>>This command will unlock name|flood|photo|member of [Group_id]

## Group setting in realm
>[!/]setting [Group_id]
>>[!/]setting 12345678
>>>This command will return settings of [Group_id]

### Add admin
>[!/]addadmin [username]
>>[!/]addadmin @username
>>>This command will add username as admin
>>>
>>>_Only works in realms_

### Remove admin
>[!/]removeadmin [username]
>
>[!/]removeadmin [username]
>
>>[!/]removeadmin @username
>>>This command will add username as admin
>>>
>>>_Only works in realms_ [username]

### Admins|group list in realm
>[!/]ist [admins|groups]
>>[!/]list groups
>>>This command will return admins|groups list

### Broadcast
>[!/]broadcast [text]
>>[!/]broadcast Hello !
>>>This command will send text to all groups
>>>
>>>_Only sudo users can run this command_
>
>/br [group_id] [text]
>>/br 123456789 Hello !
>>>This command will send text to [group_id]

##Global ban commands
>[!/]banall [id]
>>[!/]banall 123456789
>>>This commands will globally ban [id]
>[!/]unbanall [id]
>>[!/]unbanall 123456789
>>>This commands will remove [id] from global bans
>
>[!/]gbanlist
>>This command will return global bans ids
>
>[!/]banlist [group_id]
>>[!/]banlist 123456789
>>>This command will return banned user Of [group_id]

### Set group owner
>[!/]setgpowner [group_id] [User_id]
>>[!/]setgpowner 123456789 987654321
>>>This command will set [User_id] as the owner of [group_id]

### Bot stats 
>[!/]stats teleseed
>>This command will return bot stats

# Realm Help
>/[!/]help
>>Get realm commands list




## owners and mods commands

_Sudo users and admins can also use this commands in all groups_

### Hammer

>[!/]kick [username|id]
>>[!/]kick @useranme
>>[!/]kick 123456789
>>>This command will remove that user
>
>[!/]ban [username|id]
>>[!/]ban @username
>>[!/]ban 123456789
>>>this command will ban and remove that user
>
>[!/]unban [id]
>>[!/]unban 12345678
>>>This command will unban that user
>
>[!/]banlist
>>This command will return bans list

### group name|member|arabic|flood|bots lock
>[!/]lock [name|member|arabic|flood|bots]
>>[!/]lock flood
>>>This command will lock name|member|arabic|flood|bots of groups

### group name|member|arabic|flood|bots unlock
>[!/]unlock [name|member|arabic|flood|bots]
>>[!/]unlock flood
>>>This command will unlock name|member|arabic|flood|bots of groups

### Group modlist|rules|about|member clean
>[!/]clean [modlist|rules|about|member]
>>[!/]clean modlist
>>>This command will clean modlist|rules|about|member
>>>_/clean member will kick all users except owner,admins and bot and it's for owners only_

### Set groups rules|about
>[!/]set [rules|about] [text]
>>[!/]set rules don't spam!
>>
>>No NSFW
>>> This command will set [text] as the rules|about of groups

### Setting or changing group owner
>[!/]setowner [id]
>>[!/]setowner 123456789
>>>This command will set id as owner of that group

### Setting or changing group photo
>[!/]setphoto
>> This command will change or set group photo
>>_also locks photo_

### Changing Group name
>[!/]setname [name]
>>[!/]setname SEED
>>>This command will set [name] as name of groups

### Group link
>[!/]newlink 
>>This command will revoke group link
>
>[!/]link
>>This command will return group link

### Promote and demote mods
>[!/]promote [username]
>>[!/]promote @username
>>>This command will promote @username as moderator
>
>[!/]demote [username]
>>[!/]demote @username
>>> This command will demote @username

### Resolve username
>[!/]res [username]
>>[!/]res @username
>>>This command will return info about that username

### Flood sensitivity
>[!/]setflood [value]
>>[!/]set flood 15  
>>> will set flood sensitivity to [value]

### Group rules and about
>[!/]about
>>This command will return group description
>
>[!/]rules
>>This command will return group rules
>>>_normal users can use it too_

### Group settings
>[!/]setting
>>This command will return group settings

### modlist
>[!/]modlist
>>This command will return group moderators
>>>_normal users can use it too_

###Help 
>[!/]help

### Owner
>[!/]owner 
>>This command will return owners id

### Save and get
>[!/]save [title] [text]
>>[!/]save spam Don't spam !
>>>This command will save text as that title
>
>[!/]get [title]
>>[!/]get spam
>>>This command will return text of that title

### Id
>[!/]id
>>This command will return user or group id
>>_can be triggered by reply_
>>
>>_Normal users can use it_ 

### group stats
>[!/]stats 
>>This command will return group message statistic in a .txt file
>
>[!/]statslist
>>This command will return group message statistic

### member list
>[!/]who
>>This command will return member list in a .txt file
>
>[!/]wholist
>>This command will return member list

# Group Help
>/[!/]help
>>Get commands list



## in private commands

**These commands only works in bots private**

### Hammer
>[!/]owners group_id [kick|ban|unban] user_id
>>[!/]owners 1234567 kick 1234567

### cleaning
>[!/]owners group_id clean  [modlist|rules|about]
>>[!/]owners 1234567 clean modlist

### setting flood sensitivity
>[!/]owners group_id setflood value
>>[!/]owners 1234567 setflood 17

### lock groups member|name
>[!/]owners group_id lock [member|name]
>>[!/]owners 1234567 lock member

#### unlock groups member|name
>[!/]owner group_id unlock [member|name]
>>[!/]owners 1234567 unlock name

### Group link
>[!/]owners group_id get link
>>[!/]owners 1234567 get link
>
>[!/]owners group_id new link
>>[!/]owners 1234567 new link

### change name|rules|name
>[!/]changename [group_id] [name]
>>[!/]changename 123456789 SEED
>
>[!/]changrules [group_id] [rules]
>>[!/]changrules 123456789 rules !
>
>/changeabout [group_id] [about]
>>/changeabout 123456789 about !

### Group log
>/loggroup [group_id]
>>/loggroup 123456789

### Join
>/[!/]oin [group_id]
>> This command will add user in [group_id]


**U can use both "/" and "!"**

# Installation 

```bash
# Tested on Ubuntu 14.04, for other OSs check out https://github.com/yagop/telegram-bot/wiki/Installation
sudo apt-get install libreadline-dev libconfig-dev libssl-dev lua5.2 liblua5.2-dev libevent-dev make unzip git redis-server g++ libjansson-dev libpython-dev expat libexpat1-dev
```

```bash
# lets install the bot
cd $HOME
git clone https://github.com/SEEDTEAM/TeleSeed.git
cd TeleSeed
chmod +x launch.sh
./launch.sh install
./launch.sh # Enter a phone number & confirmation code.
```
### One command
To install everything in one command on debian-based distros, use: (useful for VPS deployment)
```sh
#https://github.com/yagop/telegram-bot/wiki/Installation
sudo apt-get update; sudo apt-get upgrade -y --force-yes; sudo apt-get dist-upgrade -y --force-yes; sudo apt-get install libreadline-dev libconfig-dev libssl-dev lua5.2 liblua5.2-dev libevent-dev libjansson* libpython-dev make unzip git redis-server g++ -y --force-yes && git clone https://github.com/SEEDTEAM/TeleSeed.git && cd TeleSeed && chmod +x launch.sh && ./launch.sh install && ./launch.sh
```
### Realm configuration 

After you ran bot for first time, go to bot and use !id command

Get your id then, stop bot

open file ./data/config.lua 

add your id to the "sudo_users" section in the following format:
```
  sudo_users = {
    110626080,
    103649648,
    111020322,
    0,
    YourID
  }
```
then start the bot again

Create a realm using !createrealm command


# Support and development

Do not contact **us** In private for support.
Join our bot development group by sending `/join 56670147` to [@TeleSeed](https://telegram.me/TeleSeed)

_If you are using /join service for first time you have to send it 11 times ( Bot is spam reported )_

# Special thanks to
[@seyedan25](https://telegram.me/seyedan25)

For Managing [@teleseed](https://telegram.me/TeleSeed) on Telegram

[@Vamptacus](https://telegram.me/Vamptacus)

For graphic designs

[@topkecleon](https://github.com/topkecleon)

[@JuanPotato](https://github.com/JuanPotato)

# Our team!

[Alphonse](https://github.com/hmon) ([Telegram](https://telegram.me/iwals))

[I M /-\ N](https://github.com/imandaneshi) ([Telegram](https://telegram.me/imandaneshi))

[Siyanew](https://github.com/Siyanew) ([Telegram](https://telegram.me/Siyanew))

[Rondoozle](https://github.com/Rondoozle) ([Telegram](https://telegram.me/Rondoozle))

Our Telegram cahnnels > English: [@TeleSeedCH](https://telegram.me/teleseedch) Persian: [@IranSeed](https://telegram.me/iranseed)
