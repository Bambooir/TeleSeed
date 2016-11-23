while true; do
    echo "Killed TeleSeed" >> /dev/null 2>&1
    kill -9 $(pgrep telegram-cli) >> /dev/null 2>&1
    sleep 500 >> /dev/null 2>&1
    clear >> /dev/null 2>&1
done
