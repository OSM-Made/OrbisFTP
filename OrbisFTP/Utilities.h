#pragma once
#include <sys/uio.h>

enum NotifyType
{
	NotificationRequest = 0,
	SystemNotification = 1,
	SystemNotificationWithUserId = 2,
	SystemNotificationWithDeviceId = 3,
	SystemNotificationWithDeviceIdRelatedToUser = 4,
	SystemNotificationWithText = 5,
	SystemNotificationWithTextRelatedToUser = 6,
	SystemNotificationWithErrorCode = 7,
	SystemNotificationWithAppId = 8,
	SystemNotificationWithAppName = 9,
	SystemNotificationWithAppInfo = 9,
	SystemNotificationWithAppNameRelatedToUser = 10,
	SystemNotificationWithParams = 11,
	SendSystemNotificationWithUserName = 12,
	SystemNotificationWithUserNameInfo = 13,
	SendAddressingSystemNotification = 14,
	AddressingSystemNotificationWithDeviceId = 15,
	AddressingSystemNotificationWithUserName = 16,
	AddressingSystemNotificationWithUserId = 17,

	UNK_1 = 100,
	TrcCheckNotificationRequest = 101,
	NpDebugNotificationRequest = 102,
	UNK_2 = 102,
};

struct NotifyBuffer
{ //Naming may be incorrect.
	NotifyType Type;		//0x00 
	int ReqId;				//0x04
	int Priority;			//0x08
	int MsgId;				//0x0C
	int TargetId;			//0x10
	int UserId;				//0x14
	int unk1;				//0x18
	int unk2;				//0x1C
	int AppId;				//0x20
	int ErrorNum;			//0x24
	int unk3;				//0x28
	char UseIconImageUri; 	//0x2C
	char Message[1024]; 	//0x2D
	char Uri[1024]; 		//0x42D
	char unkstr[1024];		//0x82D
}; //Size = 0xC30

#define	MNT_UPDATE	0x0000000000010000ULL

//Modules.
extern bool(*Jailbreak)();
extern void(*_sceSysmoduleLoadModuleInternal)(uint32_t); //Import is broken for some reason
bool LoadModules();

//Misc
void Notify(const char* MessageFMT, ...);
void klog(const char* fmt, ...);

//File IO.
static void build_iovec(iovec** iov, int* iovlen, const char* name, const void* val, size_t len);
int nmount(struct iovec *iov, uint32_t niov, int flags);
int unmount(const char *dir, int flags);
int mount_large_fs(const char* device, const char* mountpoint, const char* fstype, const char* mode, unsigned int flags);
void CopyFile(const char* File, const char* Destination);
