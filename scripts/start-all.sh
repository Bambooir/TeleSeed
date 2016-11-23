tmux new-session -d -s 0 "bash start.sh >> /dev/null 2>&1"
clear
tmux new-session -d -s 1 "bash kill.sh >> /dev/null 2>&1"
clear
tmux new-session -d -s 2 "bash update.sh >> /dev/null 2>&1"
sleep 5
clear
exit
