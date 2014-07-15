
cd "$1"
rm -rf ./lidbg_log
mkdir lidbg_log
IFS=$'\n';
for i in * 
do
	echo --------------------------------------------------------------------------  >> ./lidbg_log/acc_count.txt
	echo ---------------------------$i---------------------------------------------  >> ./lidbg_log/acc_count.txt
	echo --------------------------------------------------------------------------  >> ./lidbg_log/acc_count.txt
	cat $i | grep "fastboot_resume:" >>  ./lidbg_log/acc_count.txt

	echo ------------------------------------------------------------------------  >> ./lidbg_log/block_clk.txt
	echo --------------------------$i--------------------------------------------  >> ./lidbg_log/block_clk.txt
	echo ------------------------------------------------------------------------  >> ./lidbg_log/block_clk.txt
	cat $i | grep "block unsafe clk:" >>  ./lidbg_log/block_clk.txt

	echo ------------------------------------------------------------------------  >>  ./lidbg_log/reset_count.txt
	echo --------------------------$i--------------------------------------------  >>  ./lidbg_log/reset_count.txt
	echo ------------------------------------------------------------------------  >>  ./lidbg_log/reset_count.txt
	cat $i | grep "Linux version" >>  ./lidbg_log/reset_count.txt

	echo ------------------------------------------------------------------------  >>  ./lidbg_log/kill_process.txt
	echo --------------------------$i--------------------------------------------  >>  ./lidbg_log/kill_process.txt
	echo ------------------------------------------------------------------------  >>  ./lidbg_log/kill_process.txt
	cat $i | grep "killing any children in process group" >>  ./lidbg_log/kill_process.txt

	echo ------------------------------------------------------------------------  >>  ./lidbg_log/bp_my.txt
	echo --------------------------$i--------------------------------------------  >>  ./lidbg_log/bp_my.txt
	echo ------------------------------------------------------------------------  >>  ./lidbg_log/bp_my.txt
	cat $i | grep "bp:my" >>  ./lidbg_log/bp_my.txt

	echo ------------------------------------------------------------------------  >>  ./lidbg_log/bp_pwr.txt
	echo --------------------------$i--------------------------------------------  >>  ./lidbg_log/bp_pwr.txt
	echo ------------------------------------------------------------------------  >>  ./lidbg_log/bp_pwr.txt
	cat $i | grep "bp:pwr=0x1" >> ./lidbg_log/bp_pwr.txt

	echo ------------------------------------------------------------------------  >>  ./lidbg_log/acc_correct.txt
	echo --------------------------$i--------------------------------------------  >>  ./lidbg_log/acc_correct.txt
	echo ------------------------------------------------------------------------  >>  ./lidbg_log/acc_correct.txt
	cat $i | grep "acc_correct:send power_key" >>  ./lidbg_log/acc_correct.txt

	echo ------------------------------------------------------------------------  >>  ./lidbg_log/bp_reset.txt
	echo --------------------------$i--------------------------------------------  >>  ./lidbg_log/bp_reset.txt
	echo ------------------------------------------------------------------------  >>  ./lidbg_log/bp_reset.txt
	cat $i | grep "bp:reset" >> ./lidbg_log/bp_reset.txt

	echo ------------------------------------------------------------------------  >>  ./lidbg_log/wakelock.txt
	echo --------------------------$i--------------------------------------------  >>  ./lidbg_log/wakelock.txt
	echo ------------------------------------------------------------------------  >>  ./lidbg_log/wakelock.txt
	cat $i | grep -E "block wakelock|clear user wakelock" >> ./lidbg_log/wakelock.txt

	echo ------------------------------------------------------------------------  >>  ./lidbg_log/lowmemkill.txt
	echo --------------------------$i--------------------------------------------  >>  ./lidbg_log/lowmemkill.txt
	echo ------------------------------------------------------------------------  >>  ./lidbg_log/lowmemkill.txt
	cat $i | grep "send sigkill to" >> ./lidbg_log/lowmemkill.txt

	echo ------------------------------------------------------------------------  >>  ./lidbg_log/I2C.txt
	echo --------------------------$i--------------------------------------------  >>  ./lidbg_log/I2C.txt
	echo ------------------------------------------------------------------------  >>  ./lidbg_log/I2C.txt
	cat $i | grep "I2C slave addr" >> ./lidbg_log/I2C.txt

	cat $i | grep -E "block unsafe clk:33|lock unsafe clk:34|block unsafe clk:14|block wakelock|send sigkill to|killing any children in process group|acc_correct:send power_key|bp:my|Linux version|bp:reset|bp:pwr=0x1|clear user wakelock|f8_timeout|quick_resume" >>  ./lidbg_log/$i
done

	date >>  ./lidbg_log/finish.txt

