#include "Common.h"

int main()
{
	//Load internal system modules.
	if (!LoadModules())
	{
		Notify("Failed to Load Modules...");
		goto Exit;
	}

	//load the user service.
	if(sceUserServiceInitialize(NULL) < 0)
	{
		Notify("Failed to Init User Service...");
		goto Exit;
	}

	//Break Free
	if (!Jailbreak())
	{
		Notify("Jailbreak failed...");
		goto Exit;
	}

	//Copy to directory we can install from
	CopyFile("/mnt/sandbox/OFTP00000_000/app0/OrbisFTP.pkg", "/user/app/OrbisFTP.pkg");

	//Install Pkg
	if (!Install_Package("/user/app/OrbisFTP.pkg"))
	{
		Notify("Failed to Install OrbisFTP...");
		goto Exit;
	}

	//Delete temp pkg
	sceKernelUnlink("/user/app/OrbisFTP.pkg");

	//Mount path with RW.
	mount_large_fs("/dev/da0x4.crypt", "/system", "exfatfs", "511", MNT_UPDATE);

	//Copy Files
	sceKernelMkdir("/system/vsh/app/OFTP00001", 0777);
	sceKernelMkdir("/system/vsh/app/OFTP00001/sce_sys", 0777);
	sceKernelMkdir("/system/vsh/app/OFTP00001/sce_module", 0777);
	CopyFile("/mnt/sandbox/OFTP00000_000/app0/OFTP00001/eboot.bin", "/system/vsh/app/OFTP00001/eboot.bin");
	CopyFile("/mnt/sandbox/OFTP00000_000/app0/OFTP00001/sce_sys/param", "/system/vsh/app/OFTP00001/sce_sys/param.sfo");
	CopyFile("/mnt/sandbox/OFTP00000_000/app0/OFTP00001/sce_module/libjbc.sprx", "/system/vsh/app/OFTP00001/sce_module/libjbc.sprx");

	//Remove the installation app
	sceAppInstUtilAppUnInstall((char*)"OFTP00000");

Exit:
	sceSystemServiceLoadExec("exit", 0);

	return 0;
}