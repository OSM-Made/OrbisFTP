#include "Common.h"
#include "FTPS4.h"
#include <orbis/libkernel.h>
int run;

void custom_MTRW(ftps4_client_info_t *client)
{
	if (mount_large_fs("/dev/da0x1.crypt", "/preinst2", "exfatfs", "511", MNT_UPDATE) < 0) goto fail;
	if (mount_large_fs("/dev/da0x4.crypt", "/system", "exfatfs", "511", MNT_UPDATE) < 0) goto fail;
	if (mount_large_fs("/dev/da0x5.crypt", "/system_ex", "exfatfs", "511", MNT_UPDATE) < 0) goto fail;
	if (mount_large_fs("/dev/da0x9.crypt", "/system_data", "exfatfs", "511", MNT_UPDATE) < 0) goto fail;
	if (mount_large_fs("/dev/md0", "/", "exfatfs", "511", MNT_UPDATE) < 0) goto fail;
	if (mount_large_fs("/dev/md0.crypt", "/", "exfatfs", "511", MNT_UPDATE) < 0) goto fail;
	if (mount_large_fs("/dev/da0x0.crypt", "/preinst", "exfatfs", "511", MNT_UPDATE) < 0) goto fail;

	ftps4_ext_client_send_ctrl_msg(client, "200 Mount success." FTPS4_EOL);
	return;

fail:
	ftps4_ext_client_send_ctrl_msg(client, "550 Could not mount!" FTPS4_EOL);
}

void custom_SHUTDOWN(ftps4_client_info_t *client) {
	ftps4_ext_client_send_ctrl_msg(client, "200 Shutting down..." FTPS4_EOL);
	run = 0;
}

int get_ip_address(char *ip_address)
{
	int ret;
	OrbisNetCtlInfo info;

	ret = sceNetCtlInit();
	if (ret < 0)
		goto error;

	ret = sceNetCtlGetInfo(ORBIS_NET_CTL_INFO_IP_ADDRESS, &info);
	if (ret < 0)
		goto error;

	memcpy(ip_address, info.ip_address, sizeof(info.ip_address));

	sceNetCtlTerm();

	return ret;

error:
	ip_address = NULL;
	return -1;
}

int main()
{
	//Load Supporting Internal Dlls.
	if (!LoadModules())
	{
		Notify("Failed to Load Modules...");
		sceSystemServiceLoadExec("exit", 0);
	}

	//Jailbreak our new process and give it root.
	if (!Jailbreak())
	{
		Notify("Jailbreak failed...");
		sceSystemServiceLoadExec("exit", 0);
	}

	//Mount folders
	mount_large_fs("/dev/da0x1.crypt", "/preinst2", "exfatfs", "511", MNT_UPDATE);
	mount_large_fs("/dev/da0x4.crypt", "/system", "exfatfs", "511", MNT_UPDATE);
	mount_large_fs("/dev/da0x5.crypt", "/system_ex", "exfatfs", "511", MNT_UPDATE);
	mount_large_fs("/dev/da0x9.crypt", "/system_data", "exfatfs", "511", MNT_UPDATE);
	mount_large_fs("/dev/md0", "/", "exfatfs", "511", MNT_UPDATE);
	mount_large_fs("/dev/md0.crypt", "/", "exfatfs", "511", MNT_UPDATE);
	mount_large_fs("/dev/da0x0.crypt", "/preinst", "exfatfs", "511", MNT_UPDATE);

	char ip_address[ORBIS_NET_CTL_IPV4_ADDR_STR_LEN];
	run = 1;

	int ret = get_ip_address(ip_address);
	if (ret < 0)
		sceSystemServiceLoadExec("exit", 0);

	ftps4_init(ip_address, FTP_PORT);
	ftps4_ext_add_command("SHUTDOWN", custom_SHUTDOWN);
	ftps4_ext_add_command("MTRW", custom_MTRW);

	Notify("FTPS4 v%s Loaded!", VERSION);
	Notify("IP:     %s\nPort: %i", ip_address, FTP_PORT);

	while (true) { sceKernelUsleep(10 * 1000); }

    return 0;
}