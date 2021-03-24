#include "Common.h"
#include "Utilities.h"

#pragma region Modules

bool(*Jailbreak)();
void(*_sceSysmoduleLoadModuleInternal)(uint32_t); //Import is broken for some reason

bool LoadModules()
{
	//Loading the Firmware Agnostic Jailbreak credits to https://github.com/sleirsgoevy
	int ModuleHandle_libjbc = sceKernelLoadStartModule("/app0/sce_module/libjbc.sprx", 0, nullptr, 0, nullptr, nullptr);
	if (ModuleHandle_libjbc == 0) {
		klog("Failed to load libjbc Library.\n");
		return false;
	}

	sceKernelDlsym(ModuleHandle_libjbc, "Jailbreak", (void**)&Jailbreak);
	if (Jailbreak == nullptr) {
		klog("Failed to load Jailbreak Import.\n");
		return false;
	}


	//Load the sysmodule library and import for sceSysmoduleLoadModuleInternal for some reason wouldnt auto import.
	char Buffer[0x200];
	sprintf(Buffer, "/%s/common/lib/libSceSysmodule.sprx", sceKernelGetFsSandboxRandomWord());
	int ModuleHandle = sceKernelLoadStartModule(Buffer, 0, nullptr, 0, nullptr, nullptr);
	if (ModuleHandle == 0) {
		klog("Failed to load libSceSysmodule Library.\n");
		return false;
	}

	sceKernelDlsym(ModuleHandle, "sceSysmoduleLoadModuleInternal", (void**)&_sceSysmoduleLoadModuleInternal);
	if (_sceSysmoduleLoadModuleInternal == nullptr) {
		klog("Failed to load _sceSysmoduleLoadModuleInternal Import.\n");
		return false;
	}

	_sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_SYSTEM_SERVICE);
	_sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_USER_SERVICE);
	_sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_SYS_CORE);
	_sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_COMMON_DIALOG);
	_sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_PAD);
	_sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_NETCTL);
	_sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_NET);
	_sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_HTTP);

	return true;
}
#pragma endregion

#pragma region Misc

void Notify(const char* MessageFMT, ...)
{
	NotifyBuffer Buffer;

	//Create full string from va list.
	va_list args;
	va_start(args, MessageFMT);
	vsprintf(Buffer.Message, MessageFMT, args);
	va_end(args);

	//Populate the notify buffer.
	Buffer.Type = NotifyType::NotificationRequest; //this one is just a standard one and will print what ever is stored at the buffer.Message.
	Buffer.unk3 = 0;
	Buffer.UseIconImageUri = 1; //Bool to use a custom uri.
	Buffer.TargetId = -1; //Not sure if name is correct but is always set to -1.
	strcpy(Buffer.Uri, "https://i.imgur.com/SJPIBGG.png"); //Copy the uri to the buffer.

  //From user land we can call int64_t sceKernelSendNotificationRequest(int64_t unk1, char* Buffer, size_t size, int64_t unk2) which is a libkernel import.
	sceKernelSendNotificationRequest(0, (char*)&Buffer, 3120, 0);

	//What sceKernelSendNotificationRequest is doing is opening the device "/dev/notification0" or "/dev/notification1"
	// and writing the NotifyBuffer we created to it. Somewhere in ShellUI it is read and parsed into a json which is where
}

void klog(const char* fmt, ...)
{

}

#pragma endregion

#pragma region File IO

static void build_iovec(iovec** iov, int* iovlen, const char* name, const void* val, size_t len) {
	int i;

	if (*iovlen < 0)
		return;

	i = *iovlen;
	*iov = (iovec*)realloc(*iov, sizeof **iov * (i + 2));
	if (*iov == NULL) {
		*iovlen = -1;
		return;
	}

	(*iov)[i].iov_base = strdup(name);
	(*iov)[i].iov_len = strlen(name) + 1;
	++i;

	(*iov)[i].iov_base = (void*)val;
	if (len == (size_t)-1) {
		if (val != NULL)
			len = strlen((const char*)val) + 1;
		else
			len = 0;
	}
	(*iov)[i].iov_len = (int)len;

	*iovlen = ++i;
}

int nmount(struct iovec *iov, uint32_t niov, int flags)
{
	return syscall(378, iov, niov, flags);
}

int unmount(const char *dir, int flags)
{
	return syscall(22, dir, flags);
}

int mount_large_fs(const char* device, const char* mountpoint, const char* fstype, const char* mode, unsigned int flags)
{
	struct iovec* iov = NULL;
	int iovlen = 0;

	unmount(mountpoint, 0);

	build_iovec(&iov, &iovlen, "fstype", fstype, -1);
	build_iovec(&iov, &iovlen, "fspath", mountpoint, -1);
	build_iovec(&iov, &iovlen, "from", device, -1);
	build_iovec(&iov, &iovlen, "large", "yes", -1);
	build_iovec(&iov, &iovlen, "timezone", "static", -1);
	build_iovec(&iov, &iovlen, "async", "", -1);
	build_iovec(&iov, &iovlen, "ignoreacl", "", -1);

	if (mode) {
		build_iovec(&iov, &iovlen, "dirmask", mode, -1);
		build_iovec(&iov, &iovlen, "mask", mode, -1);
	}

	return nmount(iov, iovlen, flags);
}

void CopyFile(const char* File, const char* Destination)
{
	int src = 0, dst = 0;
	OrbisKernelStat Stats;

	//Open file descriptors 
	src = sceKernelOpen(File, SCE_KERNEL_O_RDONLY, 0);
	if (src <= 0)
	{
		printf("[OrbisLib Installer] Failed to open Source File.");
		return;
	}

	dst = sceKernelOpen(Destination, SCE_KERNEL_O_CREAT | SCE_KERNEL_O_WRONLY, 0777);
	if (dst <= 0)
	{
		printf("[OrbisLib Installer] Failed to Destination Source File.");
		return;
	}

	//Get File size
	sceKernelFstat(src, &Stats);

	if (Stats.st_size == 0)
	{
		printf("[OrbisLib Installer] Failed to get file size.");
		return;
	}

	//Allocate space to read data.
	char* FileData = (char*)malloc(Stats.st_size);

	//ReadFile.
	sceKernelRead(src, FileData, Stats.st_size);

	//Write the file data.
	sceKernelWrite(dst, FileData, Stats.st_size);

	//Close The handles.
	sceKernelClose(src);
	sceKernelClose(dst);
}

#pragma endregion