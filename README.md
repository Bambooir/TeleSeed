# [TeleSeed](https://telegram.me/TeleSeed)

[![https://gitter.im/SEEDTEAM/TeleSeed](https://badges.gitter.im/SEEDTEAM/TeleSeed.svg)](https://gitter.im/SEEDTEAM/TeleSeed?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

**An advanced and powerful administration bot based on [yagop/telegram-bot](https://github.com/yagop/telegram-bot) licensed under the [GNU General Public License](https://github.com/SEEDTEAM/TeleSeed/blob/master/LICENSE)**.
# Features

* **A powerful antispam system with custom sensitivity for each group**
* **Multiple realms (admin groups)**
* **Recalcitrant to any kind of spamming (X/Y bots, name/photo changers, etc.)**
* **Global banning**
* **Broadcast to all groups**
* **Group and  links**
* **Kick, ban and unban by reply**
* **Groups, ban and global ban list**
* **Logging anything that happens in a group**
* **Invitation by username**
* **Group administration via private messages**
* **Only mods, owner and admin can add bots in groups**
* **Arabic lock**
* **Lock TgService**
* **Chat list**
* **And more!**


* * *

## Commands
TeleSeed has several commands that are only usable at certain ranks.


## General Commands
| Command | Description |
|:--------|:------------|
| [#!/]id | Returns group/SuperGroup ID or user id in pm. |
| [#!/]teleseed | Returns bot information. |


## Private Message Commands
| Command | Description |
|:--------|:------------|
| [#!/]help | Returns help text. |
| [#!/]superhelp | Returns SuperGroup help text. |
| [#!/]chats | Returns public chats in a message(s). |
| [#!/]chatlist | Returns public chats in a .txt document. |
| [#!/]join [GroupID] | Join a public chat by id |


## Moderator commands 

| Command | Description | Groups? | SuperGroups? |
|:--------|:------------|:--------|:-------------|
| [#!/]info | Returns general info about the SuperGroup. | N | Y |
| [#!/]admins | Returns SuperGroup admins list. | N | Y |
| [#!/]modlist | Returns Moderator list. | Y | Y |
| [#!/]owner | Returns group/SuperGroup owner. _Can be used by any user_| Y | Y |
| [#!/]bots | Lists bots in SuperGroup. | N | Y |
| [#!/]who | Lists all users in group *SuperGroup returns .txt document list. | Y | Y |
| [#!/]wholist | Lists all users in group in .txt document. | Y | N |
| [#!/]block | Kicks a user from SuperGroup **Adds user to blocked list**. | N | Y |
| [#!/]kick | kicks user from group. | Y | N |
| [#!/]ban | Bans user from the group/SuperGroup. | Y | Y |
| [#!/]unban | Unbans user from group/SuperGroup. | Y | Y |
| [#!/]id [username<html>&#124;</html>reply] | For userID's: !id @username (**ONLY IN SuperSroups**) or reply `!id` in any group. | Y | Y |
| [#!/]id from | Returns ID of user a message is forwarded from. | N | Y |
| [#!/]kickme | Kicks user from SuperGroup / In SuperGroup: **Must be unblocked by owner or use `#join` by pm to return** | Y | Y |
| [#!/]setowner | Sets group/SuperGroup owner. | Y | Y |
| [#!/]promote | Promote a group/SuperGroup moderator. | Y | Y |
| [#!/]demote | Demote a group/SuperGroup moderator. | Y | Y |
| [#!/]setname | Set group/SuperGroup name. | Y | Y |
| [#!/]setphoto | Set group/SuperGroup photo. | Y | Y |
| [#!/]setrules | Set group/SuperGroup rules. | Y | Y |
| [#!/]setabout |  Sets the about section in chat info(members list) | N | Y |
| [#!/]set about |  | Y | N |
| [#!/]save [value] <text> | Sets extra info for group/SuperGroup by value. | Y | Y |
| [#!/]get [value] | Returns extra info for group/SuperGroup by value. | Y | Y |
| [#!/]newlink | Generate a new group/SuperGroup link. | Y | Y |
| [#!/]setlink | Set a new SuperGroup link _If bot is not creator_. | N | Y |
| [#!/]link | Retrieves the group/SuperGroup link. | Y | Y |
| [#!/]rules | Retrieves the group/SuperGroup rules. | Y | Y |
| [#!/]lock [setting](#settings-settings) | Lock group/SuperGroup settings. | Y | Y |
| [#!/]unlock [setting](#settings-settings) | Unlock group/SuperGroup. | Y | Y |
| [#!/]mute [mute]#mutes-mutes) | mute group message types. | Y | Y |
| [#!/]unmute [mute](#mutes-mutes) | unmute group message types. | Y | Y |
| [#!/]setflood [value](#settings-settings) | Set [value] as flood sensitivity.| Y | Y |
| [#!/]muteuser [username<html>&#124;</html>id<html>&#124;</html>reply] | Mute and unmute a user in chat. ***If a muted user posts a message, the message is deleted automaically only owners can mute <html>&#124;</html> mods and owners can unmute*** | Y | Y |
| [#!/]mutelist | Returns list of muted users in group/SuperGroup. | Y | Y |
| [#!/]muteslist | Returns mutes for group/SuperGroup. | Y | Y |
| [#!/]settings | Returns a list of group/SuperGroup settings. | Y | Y |
| [#!/]stats | Returns simple message statistics in a .txt document. | Y | Y |
| [#!/]statslist | Returns simple message statistics in a message. | Y | Y |
| [#!/]banlist | Returns group/SuperGroup banlist. | Y | Y |
| [#!/]clean [rules<html>&#124;</html>about<html>&#124;</html>modlisthtml>&#124;</html>bots<html>&#124;</html>mutelist] | Clears rules, about, modlist, bots, or mutelist | Y | Y |
| [#!/]del | Deletes a message by reply. | N | Y |
| [#!/]res [username] | Returns users name and id by username. | Y | Y |
| [#!/]log | Returns group logs.***In SuperGroups:*** Search for kick reasons using [#RTL<html>&#124;</html>#spam<html>&#124;</html>#lockmember] | Y | Y |

## Owner commands

| Command | Description |   groups/SuperGroups? |  In private? |
|:--------|:------------|:----------------------|:-------------|
| [#!/]muteuser [username<html>&#124;</html>id<html>&#124;</html>reply] | Mute and unmute a user in chat.***If a muted user posts a message, the message is deleted automaically / only owners can mute / mods and owners can unmute*** |  | N |
| [#!/]all | Returns all available information about current group. | Y | N |
| [#!/]all [GroupID] | Returns all available information about group by GroupID. | Y | N |
| [#!/]owners [GroupID] [kick<html>&#124;</html>ban<html>&#124;</html>unban] [UserID] | Kick, ban, or unban a user from a group by GroupID and UserID. | N | Y |
| [#!/]owners [GroupID] clean  [modlist<html>&#124;</html>rules<html>&#124;</html>about] | Clear options by GroupID. | N | Y |
| [#!/]owners [GroupID] setflood [value] | Set flood for a group by GroupID and a value [1-5]. | N | Y |
| [#!/]owners [GroupID] lock [setting] | Lock settings for a group by GroupID. | N | Y |
| [#!/]owner [GroupID] unlock [setting] | Unlock settings for a group by GroupID. | N | Y |
| [#!/]owners [GroupID] new link | Create a new group link by GroupID.  | N | Y |
| [#!/]owners [GroupID] get link | Returns group link by GroupID. | N | Y |
| [#!/]changename [GroupID] [name] | Change a group's name by GroupID. | N | Y |
| [#!/]changrules [GroupID] [rules] | Change a group's rules by GroupID. | N  | Y |
| [#!/]changeabout [GroupID] [about] | Change a group's about by GroupID. | N | Y |
| [#!/]loggroup [GroupID] | get group logs by GroupID. | N | Y |

## Admin commands

| Command | Description | Groups? | SuperGroups? | Realms? |
|:--------|:------------|:--------|:-------------|:--------|
| [#!/]creategroup [Name] | Create a group and add it to moderation.json. | Y | Y | Y |
| [#!/]createrealm [Name] | Create a realm and remove it from moderation.json. | Y | Y | Y |
| [#!/]add | Add a group to moderation.json. | Y | Y | Y |
| [#!/]rem | Remove a group from moderation.json. | Y | Y | Y |
| [#!/]rem [GroupID] | Remove a group from moderation by GroupID. | Y | Y | Y |
| [#!/]setname [Name] | Set realm name. | N | -- | Y |
| [#!/]setabout [group<html>&#124;</html>sgroup] [GroupID] [Text] | Set a group's about text. | Y | Y | Y |
| [#!/]setrules [group<html>&#124;</html>sgroup] [GroupID] [Text] | Set a group's rules. | Y | Y | Y |
| [#!/]lock [GroupID] [setting] | Lock a group's setting. | Y | Y | Y |
| [#!/]lock [GroupID] [setting] | Lock a group's setting. | Y | Y | Y |
| [#!/]unlock [GroupID] [setting] | Unlock a group's setting. | Y | Y | Y |
| [#!/]who | Get a list of members in group/realm. | Y | Y | Y |
| [#!/]wholist | Get a .txt document list of members in group/realm. | Y | Y | Y |
| [#!/]type | Get group type. | Y | Y | Y |
| [#!/]addlog | Add a Log_SuperGroup for GBan log. | N | Y | N |
| [#!/]remlog | Remove a Log_SuperGroup for GBan log. | N | Y | N |
| [#!/]kill chat [GroupID] | Kick all users and remove a group from moderation. | Y | Y | Y |
| [#!/]kill realm [RealmID] | Kick all users and remove a realm from moderation. | Y | Y | Y |
| [#!/]banall [id<html>&#124;</html>usename] | Ban a user from all moderated groups where bot is an admin (#global_ban) | Y | Y | Y |
| [#!/]unbanall [id<html>&#124;</html>usename] | Unban a user from all moderated groups where bot is an admin (#global_ban) | Y | Y | Y |
| [#!/]gbanlist [id<html>&#124;</html>usename] | Returns a list of all globally banned user | Y | Y | Y |
| [#!/]list groups | Returns a list of all groups. | Y | Y | Y |
| [#!/]list realms | Returns a list of all realms. | Y | Y | Y |
| [#!/]whitelist | User/Bot will not be subject to message checks. | Y | Y | Y |
| [#!/]support | Promote user to support. | Y | Y | Y |
| [#!/]-support | deomote user from support. | Y | Y | Y |
| [#!/]pm [UserID] <text> | Sends a private message to a user by UserID. | Y | Y | Y |
| [#!/]import <GroupLink> | Bot joins a group by GroupLink. | Y | Y | Y |
| [#!/]pmblock [UserID] | Block a user from bot private message and bot photo. | Y | Y | Y |
| [#!/]pmunblock [UserID] | Unblock a user from bot private message and bot photo. | Y | Y | Y |
| [#!/]markread [on<html>&#124;</html>off] | Toggle bot to doubble check or not doubble check messages. | Y | Y | Y |
| [#!/]setbotphoto | Set bot photo. | Y | Y | Y |
| [#!/]contactlist | Bot will generate a list of all it's contacts***and send it by private message of command sender  | Y | Y | Y |
| [#!/]dialoglist | Bot will generate a list of all it's private message users***and send it by private message of command sender  | Y | Y | Y |
| [#!/]delcontact | Delete bot contact. | Y | Y | Y |
| [#!/]reload | Reloads all bot plugins | Y | Y | Y |
| [#!/]updateid | Adds long_id to moderation data for groups. | Y | Y | Y |
| [#!/]!bc [GroupID] [text] | This command will send text to [GroupID]| Y | Y | Y |
| [#!/]leave | Bot will leave that group and can only be re-invited by an admin with bot phone number | Y | Y | Y |
| [#!/]mp | (Mod Promote) Set user as a mod of supergroup. **TESTING** | N | Y | N |
| [#!/]md | (Mod demote) Removes user from mod of supergroup. **TESTING**| N | Y | N |

## Sudo Commands 

| Command | Groups? | SuperGroups? | Realms? |
|:--------|:--------|:-------------|---------|
| [#!/]addadmin [id<html>&#124;</html>username] | Set a user as bot admin. | Y | Y | Y |
| [#!/]removeadmin [id<html>&#124;</html>username] | Remove a user from bot admin. | Y | Y | Y |
| [#!/]sync_gbans | Sync your GBans with official @TeleSeed | Y | Y | Y |


## Settings 

| Command | Groups? | SuperGroups? |
|:--------|:--------|:-------------|
| <html>&#91;</html>#!/](un)lock links | Y | Y |
| <html>&#91;</html>#!/](un)lock flood | Y | Y |
| <html>&#91;</html>#!/]setflood [5-20] | Y | Y |
| <html>&#91;</html>#!/](un)lock bots | Y | N |
| <html>&#91;</html>#!/](un)lock spam | Y | Y |
| <html>&#91;</html>#!/](un)lock arabic | Y | Y |
| <html>&#91;</html>#!/](un)lock member | Y | Y |
| <html>&#91;</html>#!/](un)lock leave | Y | N |
| <html>&#91;</html>#!/](un)lock RTL | Y | Y |
| <html>&#91;</html>#!/](un)lock tgservice | N | Y |
| <html>&#91;</html>#!/](un)lock sticker | Y| Y |
| <html>&#91;</html>#!/]public [yes<html>&#124;</html>no] | Y | Y |
| <html>&#91;</html>#!/](un)lock strict | N | Y |

/lock [setting] and /unlock [setting]: sets allowed actions and content for groups/SuperGroups
	***/lock strict*** in a supergroup, if strict is locked users will be kicked for violations of settings or mutes

/public [yes|no]: Set group/SuperGroup visibility in pm `!chats` or `!chatlist` commands.


### Mutes

| Command |
|:--------|
| [#!/]mute audio |
| [#!/]mute video |
| [#!/]mute photo |
| [#!/]mute documents |
| [#!/]mute gifs |
| [#!/]mute all |

**Groups: If "muted" message type: user is kicked if message type is posted**

**SuperGroups: A "muted" message type is auto-deleted if posted **

### Ranks 

| Rank | Description |
|:------|:------------|
| Banned | Cannot enter the group(s). |
| User | Default rank. |
| Moderator | Can set settings and kick/ban/unban users from a group. Can unmute users. |
| Owner | Can mute users. Can promote/demote moderators. Can set SuperGroup admins. |
| Support | Can globally unban users. Acts as owner of all groups. |
| Administrator | Can globally ban/unban users. Can promote/demote owners. |
| Sudo | Can add[#!/]remove groups. Can broadcast. Can promote/demote administrators. |

Each higher status inherits the privileges of the lower status.

**You can use "#", "!", or "/" to begin all commands

* * *

# Installation

```sh
# Install dependencies.
# Tested on Ubuntu 14.04. For other OSs, check out https://github.com/yagop/telegram-bot/wiki/Installation
sudo apt-get install libreadline-dev libconfig-dev libssl-dev lua5.2 liblua5.2-dev lua-socket lua-sec lua-expat libevent-dev make unzip git redis-server autoconf g++ libjansson-dev libpython-dev expat libexpat1-dev

# Let's install the bot.
cd $HOME
git clone https://github.com/SEEDTEAM/TeleSeed.git -b supergroups
cd TeleSeed
chmod +x launch.sh
./launch.sh install
./launch.sh # Enter a phone number & confirmation code.
```
### One command
To install everything in one command (useful for VPS deployment) on Debian-based distros, use:
```sh
#https://github.com/yagop/telegram-bot/wiki/Installation
sudo apt-get update; sudo apt-get upgrade -y --force-yes; sudo apt-get dist-upgrade -y --force-yes; sudo apt-get install libreadline-dev libconfig-dev libssl-dev lua5.2 liblua5.2-dev lua-socket lua-sec lua-expat libevent-dev libjansson* libpython-dev make unzip git redis-server g++ autoconf -y --force-yes && git clone https://github.com/SEEDTEAM/TeleSeed.git -b supergroups && cd TeleSeed && chmod +x launch.sh && ./launch.sh install && ./launch.sh
```

* * *

### Realm configuration

After you run the bot for first time, send it `!id`. Get your ID and stop the bot.

Open ./data/config.lua and add your ID to the "sudo_users" section in the following format:
```
  sudo_users = {
    110626080,
    103649648,
    111020322,
    0,
    YourID
  }
```
Then restart the bot.

Create a realm using the `!createrealm` command.

* * *

**Creating a LOG SuperGroup**
	-For GBan Log

	1. Create a group using the `!creategroup` command.
	2. Add two members or bots, then use `#Tosuper` to convert to a SuperSroup.
	3. Use the `#addlog` command and your ***LOG SuperGroup(s)*** will be set.
	Note: you can set multiple Log SuperGroups

* * *

# Support and development

Check out this [tutorial by Telegram Geeks](http://telegramgeeks.com/2016/01/teleseed-tutorial/) for further assistance with setup and installation.

**Do not contact us** in private for support.
Join our bot development group by sending `/join 1047524697` to [@TeleSeed](https://telegram.me/TeleSeed)

# Special thanks to
[@seyedan25](https://telegram.me/seyedan25)

For managing [@teleseed](https://telegram.me/TeleSeed) on Telegram.

[@Vamptacus](https://telegram.me/Vamptacus)

For graphic designs.

[topkecleon](https://github.com/topkecleon)

[Juan Potato](https://github.com/JuanPotato)

* * *

# Our team!

[Alphonse](https://github.com/hmon) ([Telegram](https://telegram.me/iwals))

[I M /-\ N](https://github.com/imandaneshi) ([Telegram](https://telegram.me/imandaneshi))

[Siyanew](https://github.com/Siyanew) ([Telegram](https://telegram.me/Siyanew))

[Rondoozle](https://github.com/Rondoozle) ([Telegram](https://telegram.me/POTUS))

### Our Telegram channels:

English: [@TeleSeedCH](https://telegram.me/teleseedch)

Persian: [@IranSeed](https://telegram.me/iranseed)
