while true; do
echo updating teleseed >> /dev/null 2>&1
cd ~/Clannadbot >> /dev/null 2>&1
git pull >> /dev/null 2>&1
git checkout supergroups >> /dev/null 2>&1
sleep 7200 >> /dev/null 2>&1
done
