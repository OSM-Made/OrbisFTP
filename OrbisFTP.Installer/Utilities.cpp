#include "Common.h"
#include "Utilities.h"
#include <orbis/Net.h>
#include <sys/uio.h>

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
	_sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_BGFT);
	_sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_APPINSTUTIL);

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

#pragma region Pkg

bool AppInstUtil_Initialized = false;
bool AppInstUtil_Init()
{
	int ret = 0;

	if (AppInstUtil_Initialized)
		return true;

	ret = sceAppInstUtilInitialize();
	if (ret)
	{
		klog("sceAppInstUtilInitialize failed: 0x%08X", ret);

		AppInstUtil_Initialized = false;

		return false;
	}

	AppInstUtil_Initialized = true;

	return true;
}

static SceBgftInitParams g_bgft_init;
bool BgftService_Initialized = false;
bool BgftService_Init()
{
	int ret = 0;

	if (BgftService_Initialized)
		return true;

	memset(&g_bgft_init, 0, sizeof(g_bgft_init));
	{
		g_bgft_init.heapSize = BGFT_HEAP_SIZE;
		g_bgft_init.heap = (uint8_t*)malloc(g_bgft_init.heapSize);
		if (!g_bgft_init.heap) {
			klog("No memory for BGFT heap.\n");
			return false;
		}
		memset(g_bgft_init.heap, 0, g_bgft_init.heapSize);
	}

	ret = sceBgftServiceInit(&g_bgft_init);
	if (ret && ret != 0x80990001)
	{
		klog("sceBgftServiceInit failed: 0x%08X", ret);

		if (g_bgft_init.heap) {
			free(g_bgft_init.heap);
			g_bgft_init.heap = NULL;
		}

		memset(&g_bgft_init, 0, sizeof(g_bgft_init));

		BgftService_Initialized = false;

		return false;
	}

	BgftService_Initialized = true;

	return true;
}

bool AppInstUtil_GetTitleID(const char* Path, char* TitleID)
{
	int is_app = 0;
	int ret = sceAppInstUtilGetTitleIdFromPkg(Path, TitleID, &is_app);
	if (ret) {
		klog("sceAppInstUtilGetTitleIdFromPkg failed: 0x%08X", ret);
		return false;
	}
	return true;
}

#define PKG_TITLE_ID_SIZE 0x9
#define PKG_SERVICE_ID_SIZE 0x13
#define PKG_CONTENT_ID_SIZE 0x24
#define PKG_LABEL_SIZE 0x10
#define PKG_DIGEST_SIZE 0x20
#define PKG_MINI_DIGEST_SIZE 0x14

#define SCE_BGFT_ERROR_INVALID_ARGUMENT 0x80990004

struct pkg_content_info {
	char content_id[PKG_CONTENT_ID_SIZE + 1];
	char service_id[PKG_SERVICE_ID_SIZE + 1];
	char title_id[PKG_TITLE_ID_SIZE + 1];
	char label[PKG_LABEL_SIZE + 1];
};

bool BgftService_RegisterTask(SceBgftTaskId* Task_ID, const char* Path, const char* Content_Name, char* Title_ID, SceBgftTaskOpt Option)
{
	SceBgftDownloadParam Params;
	OrbisUserServiceUserId User_ID;
	int Result;

	Result = sceUserServiceGetForegroundUser(&User_ID);
	if (Result) {
		klog("sceUserServiceGetForegroundUser failed: 0x%08X\n", Result);
		return false;
	}

	memset(&Params, 0, sizeof(Params));
	{ //Hard code cuz lazy but also the package wont change
		Params.entitlementType = 5;
		Params.userId = User_ID;
		Params.id = "IV0000-OFTP00001_00-ORBISFTP00000000";
		Params.contentUrl = Path;
		Params.contentName = Content_Name;
		Params.iconPath = "https://i.imgur.com/SJPIBGG.png";
		Params.playgoScenarioId = "0";
		Params.option = Option;
		Params.packageType = "PS4GD";
		Params.packageSubType = "";
		Params.packageSize = 0x650000;
	}

	*Task_ID = SCE_BGFT_INVALID_TASK_ID;

	Result = sceBgftServiceIntDownloadRegisterTaskByStorage(&Params, Task_ID);

	if (Result)
	{
		if (Result == 0x80990088)
		{
			*Task_ID = SCE_BGFT_INVALID_TASK_ID;
			klog("Package already installed!!");
			return false;
		}
		if (Result == SCE_BGFT_ERROR_INVALID_ARGUMENT)
			klog("sceBgftDownloadRegisterTask failed: SCE_BGFT_ERROR_INVALID_ARGUMENT");
		else
			klog("sceBgftDownloadRegisterTask failed: 0x%08X", Result);

		return false;
	}

	Result = sceBgftServiceDownloadStartTask(*Task_ID);
	if (Result)
	{
		klog("sceBgftServiceDownloadStartTask failed: 0x%08X", Result);
		sceAppInstUtilAppUnInstall(Title_ID);
		return false;
	}

	return true;
}

bool Install_Package(const char* Path)
{
	int ret = 0;
	SceBgftTaskId Task_ID = SCE_BGFT_INVALID_TASK_ID;
	char Title_ID[16];

	if (!AppInstUtil_Init())
		return false;

	if (!BgftService_Init())
		return false;

	if (!AppInstUtil_GetTitleID(Path, (char*)&Title_ID))
		return false;

	if (!BgftService_RegisterTask(&Task_ID, Path, "Orbis FTP", (char*)&Title_ID, SCE_BGFT_TASK_OPT_DISABLE_CDN_QUERY_PARAM))
		return false;

	sceKernelSleep(1);

	SceBgftTaskProgress progress_info;

	while (true)
	{
		if (Task_ID < 0)
			break;

		memset(&progress_info, 0, sizeof(progress_info));
		ret = sceBgftServiceDownloadGetProgress(Task_ID, &progress_info);
		if (ret) {
			klog("sceBgftServiceDownloadGetProgress failed: 0x%08X\n", ret);
			sceAppInstUtilAppUnInstall((char*)&Title_ID);
			return false;
		}

		int percentage = (progress_info.transferred / progress_info.transferredTotal) * 100;

		if (percentage >= 100)
			break;
	}

	sceKernelSleep(2);

	return true;
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