import java.io.FileOutputStream;



	private void lidbg_printk(String msg)
	{
		final String LOG_E = "LIDBG_PRINT";
		String newmsg = "JAVA:" + msg;
		File mFile = new File("/dev/lidbg_msg");
		if (mFile.exists())
		{
			try
			{
				FileOutputStream fout = new FileOutputStream(
						mFile.getAbsolutePath());
				byte[] bytes = newmsg.getBytes();
				fout.write(bytes);
				fout.close();
			} catch (Exception e )
			{
				Log.e(LOG_E, "Failed to lidbg_printk");
			}

		} else
		{
			Log.e(LOG_E, "/dev/lidbg_msg not exist");
		}
	}


//LIDBG_PRINT("[Release] "+wl.tag + " pid=" + wl.pid + " uid=" + wl.uid + "\n");


	private void lidbg_write(String msg)
	{
		final String LOG_E = "LIDBG_WRITE";
		byte b[] = msg.getBytes();
		FileOutputStream stateOutputMsg;
		try {
		stateOutputMsg = new FileOutputStream("/dev/mlidbg0", true);
		stateOutputMsg.write(b);
		stateOutputMsg.close();
		} catch (Exception e ) {
			Log.e(LOG_E, "Failed to lidbg_write");
		}
	}

//	lidbg_write("c wakelock lock " + tag);
//	lidbg_write("c wakelock unlock " + tag);

