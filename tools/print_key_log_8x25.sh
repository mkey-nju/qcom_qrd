
cd "$1"
rm -rf ./lidbg_log
mkdir lidbg_log
IFS=$'\n';
for i in * 
do

	cat $i | grep -E "block unsafe clk:33|lock unsafe clk:34|block unsafe clk:14|block wakelock|send sigkill to|killing any children in process group|acc_correct:send power_key|bp:my|Linux version|bp:reset|bp:pwr=|clear user wakelock|f8_timeout|quick_resume|Freezing of tasks failed after|lidbgerr|LPC: RESET Wince" >>  ./lidbg_log/$i
done

date >>  ./lidbg_log/finish.txt

