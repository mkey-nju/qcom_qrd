
function combination_menu()
{
	cd $DBG_ROOT_PATH
	echo 组合选项:
	echo "[61] 完整编译lidbg并提交到产品二进制仓库           (1  3  81(1) 41 82(3))"
	echo "[62] 编译basesystem并提交到产品二进制仓库          (81(2) 23 81(3) 42 82(3))"
	echo "[63] 拷贝lidbg和basesystem到二进制仓库后生成升级包 (81(3) 41 42 43)"
	echo "[65] 编译烧写内核后重启                            (21 53 54 56)"
}

function combination_handle()
{
	case $1 in
	61)
		read -p "输入提交到二进制仓库的说明文字：" descriptors
		lidbg_clean && lidbg_build_all && depository_clean && depository_pull && depository_copy_lidbg && depository_add_push $descriptors;;
	62)
		read -p "输入提交到二进制仓库的说明文字：" descriptors
		cd $DBG_SYSTEM_DIR && expect $DBG_TOOLS_PATH/pull && soc_build_all && depository_clean && depository_pull && depository_copy_basesystem && depository_add_push $descriptors;;
	63)
	        depository_pull && depository_copy_lidbg & depository_copy_basesystem && depository_make_package && gitk&;;

	65)
		soc_build_kernel && adb wait-for-devices reboot bootloader && soc_flash_kernel && fastboot reboot;;

	*)
		echo
	esac
}

