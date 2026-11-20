/**
 * @file
 * @author Erwin Meza Vega <emezav@unicauca.edu.co>
 * @copyright MIT License
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "gpt.h"

const gpt_partition_type gpt_partition_types[] = {
	{ "No OS", "Unused / Invalid partition", "00000000-0000-0000-0000-000000000000"},
	{ "Ceph", "Multipath block write-ahead log", "01B41E1B-002A-453C-9F17-88793989FF8F"},
	{ "No OS", "MBR partition scheme", "024DEE41-33E7-11D3-9D69-0008C781F39F"},
	{ "MidnightBSD", "Unix File System (UFS) partition", "0394EF8B-237E-11E1-B4B3-E89A8F7FC3A7"},
	{ "Linux - GNU/Hurd", "Swap partition", "0657FD6D-A4AB-43C4-84E5-0933C84B4F4F"},
	{ "Fuchsia legacy partitions", "fuchsia-data", "08185F0C-892D-428A-A789-DBEEC8F55E6A"},
	{ "Linux", "Root partition (s390)", "08A7ACEA-624C-4A20-91E8-6E0FA67D23F9"},
	{ "ChromeOS", "ChromeOS miniOS", "09845860-705F-4BB5-B16C-8A8A099CAF52"},
	{ "Linux", "/usr verity signature partition for dm-verity (64-bit PowerPC big-endian)", "0B888863-D7F8-4D9E-9766-239FCE4D58AF"},
	{ "Linux", "/usr partition (mipsel: 32-bit MIPS little-endian)", "0F4868E9-9952-4706-979F-3ED3A473E947"},
	{ "Linux - GNU/Hurd", "Linux filesystem data", "0FC63DAF-8483-4772-8E79-3D69D8477DE4"},
	{ "Fuchsia standard partitions", "Factory-provisioned read-only bootloader data", "10B8DBAA-D2BF-42A9-98C6-A7C5DB3701E7"},
	{ "Android-IA", "Bootloader2", "114EAFFE-1552-4022-B26E-9B053604CF84"},
	{ "Linux", "Root verity signature partition for dm-verity (ARC)", "143A70BA-CBD3-4F06-919F-6C05683A78BC"},
	{ "Linux", "/usr partition (64-bit PowerPC little-endian)", "15BB03AF-77E7-4D4A-B12B-C0D084F7491C"},
	{ "Linux", "Root verity signature partition for dm-verity (PA-RISC)", "15DE6170-65D3-431C-916E-B0DCD8393F25"},
	{ "Ceph", "dm-crypt LUKS block DB", "166418DA-C469-4022-ADF4-B30AFD37F176"},
	{ "Linux", "Root verity partition for dm-verity (mips64el: 64-bit MIPS little-endian)", "16B417F8-3E06-4F57-8DD2-9B5232F41AA6"},
	{ "Linux", "/usr verity signature partition for dm-verity (s390)", "17440E4F-A8D0-467F-A46E-3912AE6EF2C5"},
	{ "Android 6.0+ ARM", "Android EXT", "193D1EA4-B3CA-11E4-B075-10604B889DCF"},
	{ "Android 6.0+ ARM", "Android Meta", "19A710A2-B3CA-11E4-B026-10604B889DCF"},
	{ "Linux", "Root partition (PA-RISC)", "1AACDB3B-5444-4138-BD9E-E5C2239B2346"},
	{ "Linux", "Root verity signature partition for dm-verity (32-bit PowerPC)", "1B31B5AA-ADD9-463A-B2ED-BD467FC857E7"},
	{ "Fuchsia legacy partitions", "misc", "1D75395D-F2C6-476B-A8B7-45CC1C97B476"},
	{ "Linux", "Root partition (32-bit PowerPC)", "1DE3F1EF-FA98-47B5-8DCD-4A860A654D78"},
	{ "Android-IA", "Metadata", "20AC26BE-20B7-11E3-84C5-6CFDB94711E9"},
	{ "No OS", "BIOS boot partition", "21686148-6449-6E6F-744E-656564454649"},
	{ "Fuchsia legacy partitions", "Zircon boot image (slot B)", "23CC04DF-C278-4CE7-8471-897D1A4BCDF7"},
	{ "Linux", "Root verity partition for dm-verity (ARC)", "24B2D975-0F97-4521-AFA1-CD531E421B8D"},
	{ "Android-IA", "Bootloader", "2568845D-2332-4675-BC39-8FA5A4748D15"},
	{ "Fuchsia legacy partitions", "fuchsia-blob", "2967380E-134C-4CBB-B6DA-17E7CE1CA45D"},
	{ "Linux", "Root verity partition for dm-verity (x86-64)", "2C7357ED-EBD2-46D9-AEC1-23D437EC2BF5"},
	{ "Linux", "/usr partition (64-bit PowerPC big-endian)", "2C9739E2-F068-46B3-9FD0-01C5A9AFBCCA"},
	{ "NetBSD", "Concatenated partition", "2DB519C4-B10F-11DC-B99B-0019D1879648"},
	{ "NetBSD", "Encrypted partition", "2DB519EC-B10F-11DC-B99B-0019D1879648"},
	{ "ChromeOS", "ChromeOS future use", "2E0A753D-9E48-43B0-8337-B15192CB1B5E"},
	{ "SoftRAID", "SoftRAID_Scratch", "2E313465-19B9-463F-8126-8A7993773801"},
	{ "Linux", "/usr verity partition for dm-verity (TILE-Gx)", "2FB4BF56-07FA-42DA-8132-6B139F2026AE"},
	{ "Ceph", "dm-crypt block write-ahead log", "306E8683-4FE2-4330-B7C0-00A917C16966"},
	{ "Ceph", "Block DB", "30CD0809-C2B2-499C-8879-2D6B78529876"},
	{ "Linux", "/usr verity partition for dm-verity (s390x)", "31741CC4-1A2A-4111-A581-E00B447D2D06"},
	{ "Linux", "Root verity signature partition for dm-verity (s390)", "3482388E-4254-435A-A241-766A065F9960"},
	{ "Windows", "IBM General Parallel File System (GPFS) partition", "37AFFC90-EF7D-4E96-91C3-2D7AE055B174"},
	{ "Linux", "Root partition (mipsel: 32-bit MIPS little-endian)", "37C58C8A-D913-4156-A25F-48B1B64E07F0"},
	{ "Container Linux by CoreOS", "Resizable rootfs (coreos-resize)", "3884DD41-8582-4404-B9A8-E9B84F2DF50E"},
	{ "Android-IA", "System", "38F428E6-D326-425D-9140-6E0EA133647C"},
	{ "Linux", "Root verity signature partition for dm-verity (RISC-V 32-bit)", "3A112A75-8729-4380-B4CF-764D79934448"},
	{ "Linux", "/srv (server data) partition", "3B8F8425-20E0-4F3B-907F-1A25A76F98E8"},
	{ "Linux", "/usr verity partition for dm-verity (mips64el: 64-bit MIPS little-endian)", "3C3D61FE-B5F3-414D-BB71-8739A694A4EF"},
	{ "ChromeOS", "ChromeOS rootfs", "3CB8E202-3B7E-47DD-8A3C-7FF2A13CFCEC"},
	{ "U-Boot bootloader", "U-Boot environment", "3DE21764-95BD-54BD-A5C3-4ABE786F38A8"},
	{ "Linux", "/usr verity signature partition for dm-verity (mipsel: 32-bit MIPS little-endian)", "3E23CA0B-A4BC-4B4E-8087-5AB6A26AA8A9"},
	{ "ChromeOS", "ChromeOS hibernate", "3F0F8318-F146-4E6B-8222-C28C8F02E0D5"},
	{ "Linux", "/usr verity signature partition for dm-verity (s390x)", "3F324816-667B-46AE-86EE-9B0C0C6C11B4"},
	{ "Linux", "Root verity signature partition for dm-verity (x86-64)", "41092B05-9FC8-4523-994F-2DEF0408B176"},
	{ "Android-IA", "Recovery", "4177C722-9E92-4AAB-8644-43502BFD5506"},
	{ "Fuchsia legacy partitions", "fuchsia-fvm", "41D0E340-57E3-954E-8C1E-17ECAC44CFF5"},
	{ "Fuchsia standard partitions", "Verified boot metadata (slot A/B/R)", "421A8BFC-85D9-4D85-ACDA-B64EEC0133E9"},
	{ "Haiku", "Haiku BFS", "42465331-3BA3-10F1-802A-4861696B7521"},
	{ "Darwin", "Apple Boot partition (Recovery HD)", "426F6F74-0000-11AA-AA11-00306543ECAC"},
	{ "Linux", "Root verity signature partition for dm-verity (ARM 32-bit)", "42B0455F-EB11-491D-98D3-56145BA9D037"},
	{ "Linux", "/usr partition (IA-64)", "4301D2A6-4E3B-4B2A-BB94-9E0B2C4225EA"},
	{ "Linux", "Root partition (x86)", "44479540-F297-41B2-9AF7-D131D5F0458A"},
	{ "Linux", "/usr verity signature partition for dm-verity (PA-RISC)", "450DD7D1-3224-45EC-9CF2-A43A346D71EE"},
	{ "Ceph", "Multipath journal", "45B0969E-8AE0-4982-BF9D-5A8D867AF560"},
	{ "Ceph", "dm-crypt Journal/ LUKS journal", "45B0969E-9B03-4F30-B4C6-35865CEFF106"},
	{ "Linux", "/usr verity partition for dm-verity (mipsel: 32-bit MIPS little-endian)", "46B98D8D-B55C-4E8F-AAB3-37FCA7F80752"},
	{ "barebox bootloader", "barebox-state", "4778ED65-BF42-45FA-9C5B-287A1DC4AAB1"},
	{ "Fuchsia legacy partitions", "fuchsia-install", "48435546-4953-2041-494E-5354414C4C52"},
	{ "macOS", "Hierarchical File System Plus (HFS+) partition", "48465300-0000-11AA-AA11-00306543ECAC"},
	{ "Android-IA", "Boot", "49A4D17F-93A3-45C1-A0DE-F50B2EBE2599"},
	{ "NetBSD", "Swap/FFS/LFS/RAID partition", "49F48D32-B10E-11DC-B99B-0019D1879648"},
	{ "Fuchsia standard partitions", "Fuchsia Volume Manager", "49FD7CB8-DF15-4E73-B9D9-992070127F0F"},
	{ "Darwin", "Apple Label", "4C616265-6C00-11AA-AA11-00306543ECAC"},
	{ "Fuchsia legacy partitions", "sys-config", "4E5E989E-4C86-11E8-A15B-480FCF35F8E6"},
	{ "Linux", "/usr verity signature partition for dm-verity (TILE-Gx)", "4EDE75E2-6CCC-4CC8-B9C7-70334B087510"},
	{ "Linux", "Root partition (x86-64)", "4F68BCE3-E8CD-4DB1-96E7-FBCAF984B709"},
	{ "Ceph", "Multipath OSD", "4FBD7E29-8AE0-4982-BF9D-5A8D867AF560"},
	{ "Ceph", "OSD", "4FBD7E29-9D25-41B8-AFD0-062C0CEFF05D"},
	{ "Ceph", "dm-crypt LUKS OSD", "4FBD7E29-9D25-41B8-AFD0-35865CEFF05D"},
	{ "Ceph", "dm-crypt OSD", "4FBD7E29-9D25-41B8-AFD0-5EC00CEFF05D"},
	{ "FreeBSD", "BSD disklabel/Swap/UFS/ZFS/Vinum volume manager partition", "516E7CB4-6ECF-11D6-8FF8-00022D09712B"},
	{ "Darwin", "Apple RAID/APFS/TV Recovery/HPS+FileVault/UFS Container partition", "52414944-5F4F-11AA-AA11-00306543ECAC"},
	{ "Linux", "/usr partition (TILE-Gx)", "55497029-C7C1-44CC-AA39-815ED1558630"},
	{ "Windows", "Storage Replica partition", "558D43C5-A1AC-43C0-AAC8-D1472B2923D1"},
	{ "Windows", "Logical Disk Manager (LDM) metadata partition", "5808C8AA-7E8F-42E0-85D2-E1E90434CFB3"},
	{ "Linux", "/usr verity partition for dm-verity (PA-RISC)", "5843D618-EC37-48D7-9F12-CEA8E08768B2"},
	{ "Linux", "Root verity signature partition for dm-verity (x86)", "5996FC05-109C-48DE-808B-23FA0830B676"},
	{ "Fuchsia legacy partitions", "factory-config", "5A3A90BE-4C86-11E8-A15B-480FCF35F8E6"},
	{ "Linux", "Root verity signature partition for dm-verity (LoongArch 64-bit)", "5AFB67EB-ECC8-4F85-AE8E-AC1E7C50E7D0"},
	{ "Linux", "/usr verity signature partition for dm-verity (Alpha)", "5C6E1C76-076A-457A-A0FE-F3B4CD21CE6E"},
	{ "Ceph", "Block write-ahead log", "5CE17FCE-4087-4169-B7FF-056CC58473F9"},
	{ "Container Linux by CoreOS", "/usr partition (coreos-usr)", "5DFBF5F4-2848-4BAC-AA5E-0D9A20B745A6"},
	{ "Fuchsia legacy partitions", "bootloader", "5ECE94FE-4C86-11E8-A15B-480FCF35F8E6"},
	{ "Linux", "Root partition (s390x)", "5EEAD9A9-FE09-4A1E-A1D7-520D00531306"},
	{ "Fuchsia legacy partitions", "fuchsia-system", "606B000B-B7C7-4653-A7D5-B737332C899D"},
	{ "Linux", "Root partition (RISC-V 32-bit)", "60D5A7FE-8E7D-435C-B714-3DD8162144E1"},
	{ "Linux", "Root partition (Alpha)", "6523F8AE-3EB1-4E2A-A05A-18B695AE656F"},
	{ "Darwin", "Apple APFS Preboot partition", "69646961-6700-11AA-AA11-00306543ECAC"},
	{ "Linux", "Root partition (ARM 32-bit)", "69DAD710-2CE4-4E3C-B16C-21A1D49ABED3"},
	{ "Fuchsia legacy partitions", "Verified boot metadata (slot R)", "6A2460C3-CD11-4E8B-80A8-12CCE268ED0A"},
	{ "Linux", "/usr verity partition for dm-verity (IA-64)", "6A491E03-3BE7-4545-8E38-83320E0EA880"},
	{ "Solaris", "Boot partition", "6A82CB45-1DD2-11B2-99A6-080020736631"},
	{ "illumos", "Root partition", "6A85CF4D-1DD2-11B2-99A6-080020736631"},
	{ "illumos", "Swap partition", "6A87C46F-1DD2-11B2-99A6-080020736631"},
	{ "Darwin - illumos", "ZFS/usr partition", "6A898CC3-1DD2-11B2-99A6-080020736631"},
	{ "illumos", "Backup partition", "6A8B642B-1DD2-11B2-99A6-080020736631"},
	{ "illumos", "Reserved partition", "6A8D2AC7-1DD2-11B2-99A6-080020736631"},
	{ "illumos", "/var partition", "6A8EF2E9-1DD2-11B2-99A6-080020736631"},
	{ "illumos", "/home partition", "6A90BA39-1DD2-11B2-99A6-080020736631"},
	{ "illumos", "Alternate sector", "6A9283A5-1DD2-11B2-99A6-080020736631"},
	{ "illumos", "Reserved partition", "6A945A3B-1DD2-11B2-99A6-080020736631"},
	{ "illumos", "Reserved partition", "6A96237F-1DD2-11B2-99A6-080020736631"},
	{ "illumos", "Reserved partition", "6A9630D1-1DD2-11B2-99A6-080020736631"},
	{ "illumos", "Reserved partition", "6A980767-1DD2-11B2-99A6-080020736631"},
	{ "Linux", "Root verity signature partition for dm-verity (AArch64)", "6DB69DE6-29F4-4758-A7A5-962190F00CE3"},
	{ "Linux", "/usr verity partition for dm-verity (AArch64)", "6E11A4E7-FBCA-4DED-B9E9-E1A512BB664E"},
	{ "Linux", "/usr verity signature partition for dm-verity (32-bit PowerPC)", "7007891D-D371-4A80-86A4-5CB875B9302E"},
	{ "Linux", "Root partition (mips64el: 64-bit MIPS little-endian)", "700BDA43-7A34-4507-B179-EEB93D7A7CA3"},
	{ "Linux", "Root partition (RISC-V 64-bit)", "72EC70A6-CF74-40E6-BD49-4BDA08E8F224"},
	{ "Atari TOS", "Basic data partition (GEM, BGM, F32)", "734E5AFE-F61A-11E6-BC64-92361F002671"},
	{ "Linux", "Root verity partition for dm-verity (ARM 32-bit)", "7386CDF2-203C-47A9-A498-F2ECCE45A2D6"},
	{ "Open Network Install Environment (ONIE)", "Boot", "7412F7D5-A156-4B13-81DC-867174929325"},
	{ "FreeBSD", "nandfs partition", "74BA7DD9-A689-11E1-BD04-00E081286ACF"},
	{ "Linux", "/usr partition (x86)", "75250D76-8CC6-458E-BD66-BD47CC81A812"},
	{ "HP-UX", "Data partition", "75894C1E-3AEB-11D3-B7C1-7B03A0000000"},
	{ "Android-IA", "Fastboot / Tertiary", "767941D0-2085-11E3-AD3B-6CFDB94711E9"},
	{ "Linux", "Root partition (LoongArch 64-bit)", "77055800-792C-4F94-B39A-98C91B762BB6"},
	{ "Linux", "Per-user home partition", "773F91EF-66D4-49B5-BD83-D683BF40AD16"},
	{ "Linux", "/usr verity partition for dm-verity (x86-64)", "77FF5F63-E7B6-4633-ACF4-1565B864C0E6"},
	{ "Linux", "/usr partition (ARC)", "7978A683-6316-4922-BBEE-38BFF5A2FECC"},
	{ "Linux", "Root verity partition for dm-verity (s390)", "7AC63B47-B25C-463B-8DF8-B4A94E6C90E1"},
	{ "Darwin", "Apple APFS container", "7C3457EF-0000-11AA-AA11-00306543ECAC"},
	{ "Darwin", "APFS FileVault volume container", "7C3457EF-0000-11AA-AA11-00306543ECAC"},
	{ "Storage Performance Development Kit (SPDK)", "SPDK block device", "7C5222BD-8F5D-4087-9C00-BF9843C7B58C"},
	{ "Linux", "/usr partition (ARM 32-bit)", "7D0359A3-02B3-4F0A-865C-654403E70625"},
	{ "Linux", "/usr partition (32-bit PowerPC)", "7D14FEC5-CC71-415D-9D6C-06BF0B3C3EAF"},
	{ "Ceph", "Multipath block", "7F4A666A-16F3-47A2-8445-152EF4D03F6C"},
	{ "Linux", "Plain dm-crypt partition", "7FFEC5C9-2D00-49B7-8941-3EA10A5586B7"},
	{ "OpenBSD", "Data partition", "824CC7A0-36A8-11E3-890A-952519AD3F61"},
	{ "FreeBSD", "Boot partition", "83BD6B9D-7F41-11DC-BE0B-001560B84F0F"},
	{ "Linux", "/usr partition (x86-64)", "8484680C-9521-48C6-9C11-B0720656F69E"},
	{ "MidnightBSD", "Data partition", "85D5E45A-237C-11E1-B4B3-E89A8F7FC3A7"},
	{ "MidnightBSD", "Swap partition", "85D5E45B-237C-11E1-B4B3-E89A8F7FC3A7"},
	{ "MidnightBSD", "Vinum volume manager partition", "85D5E45C-237C-11E1-B4B3-E89A8F7FC3A7"},
	{ "MidnightBSD", "ZFS partition", "85D5E45D-237C-11E1-B4B3-E89A8F7FC3A7"},
	{ "MidnightBSD", "Boot partition", "85D5E45E-237C-11E1-B4B3-E89A8F7FC3A7"},
	{ "Ceph", "dm-crypt LUKS block write-ahead log", "86A32090-3647-40B9-BBBD-38D8C573AA86"},
	{ "Linux", "Root verity partition for dm-verity (IA-64)", "86ED10D5-B607-45BB-8957-D350F23D0571"},
	{ "Ceph", "dm-crypt disk in creation", "89C57F98-2FE5-4DC0-89C1-5EC00CEFF2BE"},
	{ "Ceph", "Disk in creation", "89C57F98-2FE5-4DC0-89C1-F3AD0CEFF2BE"},
	{ "Linux", "/usr partition (s390x)", "8A4F5770-50AA-4ED3-874A-99B710DB6FEA"},
	{ "Fuchsia legacy partitions", "guid-test", "8B94D043-30BE-4871-9DFA-D69556E8C1F3"},
	{ "VeraCrypt", "Encrypted data partition", "8C8F8EFF-AC95-4770-814A-21994F2DBC8F"},
	{ "Linux", "/usr verity partition for dm-verity (Alpha)", "8CCE0D25-C0D0-4A44-BD87-46331BF1DF67"},
	{ "Linux", "Reserved", "8DA63339-0007-60C0-C436-083AC8230908"},
	{ "Linux", "/usr verity signature partition for dm-verity (IA-64)", "8DE58BC2-2A43-460D-B14E-A76E4A17B47F"},
	{ "Linux", "/usr verity partition for dm-verity (RISC-V 64-bit)", "8F1056BE-9B05-47C4-81D6-BE53128E5B54"},
	{ "Linux", "/usr verity partition for dm-verity (x86)", "8F461B0D-14EE-4E81-9AA9-049B6FB97ABD"},
	{ "Android-IA", "Factory", "8F68CC74-C5E5-48DA-BE91-A0C8C15E9C80"},
	{ "Fuchsia legacy partitions", "emmc-boot1", "900B0FC5-90CD-4D4F-84F9-9F8ED579DB88"},
	{ "Linux", "Root verity signature partition for dm-verity (mips64el: 64-bit MIPS little-endian)", "904E58EF-5C65-4A31-9C57-6AF5FC7C5DE7"},
	{ "Linux", "Root verity partition for dm-verity (64-bit PowerPC little-endian)", "906BD944-4589-4AAE-A4E4-DD983917446A"},
	{ "OS/2", "ArcaOS Type 1", "90B6FF38-B98F-4358-A21F-48F35B4A8AD3"},
	{ "Linux", "Root partition (64-bit PowerPC big-endian)", "912ADE1D-A839-4913-8964-A10EEE08FBD2"},
	{ "VMware ESX", "VMware Reserved", "9198EFFC-31C0-11DB-8F78-000C2911D1B8"},
	{ "Linux", "Root verity partition for dm-verity (64-bit PowerPC big-endian)", "9225A9A3-3C19-4D89-B4F6-EEFF88F17631"},
	{ "Linux", "/home partition", "933AC7E1-2EB4-4F13-B844-0E14E2AEF915"},
	{ "Ceph", "dm-crypt block DB", "93B0052D-02D9-4D8A-A43B-33A3EE4DFBC3"},
	{ "Linux", "/usr verity signature partition for dm-verity (ARC)", "94F9A9A1-9971-427A-A400-50CB297F0F35"},
	{ "Linux", "Root verity partition for dm-verity (TILE-Gx)", "966061EC-28E4-4B2E-B4A5-1F0A825A1D84"},
	{ "Linux", "/usr verity signature partition for dm-verity (x86)", "974A71C0-DE41-43C3-BE5D-5C5CCD1AD2C0"},
	{ "Linux", "Root verity partition for dm-verity (32-bit PowerPC)", "98CFE649-1588-46DC-B2F0-ADD147424925"},
	{ "Linux", "Root partition (IA-64)", "993D8D3D-F80E-4225-855A-9DAF8ED7EA97"},
	{ "Fuchsia standard partitions", "Zircon boot image (slot A/B/R)", "9B37FFF6-2E58-466A-983A-F7926D0B04E0"},
	{ "VMware ESX", "vmkcore (coredump partition)", "9D275380-40AD-11DB-BF97-000C2911D1B8"},
	{ "PowerPC", "PReP boot", "9E1A2D38-C612-4316-AA26-8B49521E5A8B"},
	{ "Android-IA", "Factory (alt)", "9FDAA6EF-4B3F-40D2-BA8D-BFF16BFB887B"},
	{ "Fuchsia legacy partitions", "Zircon boot image (slot R)", "A0E5CF57-2DEF-46BE-A80C-A2067C37CD49"},
	{ "Fuchsia legacy partitions", "Verified boot metadata (slot A)", "A13B4D9A-EC5F-11E8-97D8-6C3BE52705BF"},
	{ "Linux", "RAID partition", "A19D880F-05FC-4D3B-A006-743F0F84911E"},
	{ "Fuchsia legacy partitions", "Verified boot metadata (slot B)", "A288ABF2-EC5F-11E8-97D8-6C3BE52705BF"},
	{ "Fuchsia standard partitions", "Durable mutable bootloader data (including A/B/R metadata)", "A409E16B-78AA-4ACC-995C-302352621A41"},
	{ "Android-IA", "Cache", "A893EF21-E428-470A-9E55-0668FD91A2D9"},
	{ "VMware ESX", "VMFS filesystem partition", "AA31E02A-400F-11DB-9590-000C2911D1B8"},
	{ "Android-IA", "OEM", "AC6D7924-EB71-4DF8-B48D-E267B27148FF"},
	{ "Linux", "Root verity partition for dm-verity (RISC-V 32-bit)", "AE0253BE-1167-4007-AC68-43926C14C5DE"},
	{ "Windows", "Logical Disk Manager data partition", "AF9B60A0-1431-4F62-BC68-3311714A69AD"},
	{ "Linux", "/usr verity signature partition for dm-verity (LoongArch 64-bit)", "B024F315-D330-444C-8461-44BBDE524E99"},
	{ "Linux", "/usr partition (AArch64)", "B0E01050-EE5F-4390-949A-9101B17104E9"},
	{ "Fuchsia legacy partitions", "emmc-boot2", "B2B2E8D1-7C10-4EBC-A2D0-4614568260AD"},
	{ "Linux", "Root verity partition for dm-verity (s390x)", "B325BFBE-C7BE-4AB8-8357-139E652D2F6B"},
	{ "Linux", "Root verity signature partition for dm-verity (TILE-Gx)", "B3671439-97B0-4A53-90F7-2D5A8F3AD47B"},
	{ "Linux", "/usr verity partition for dm-verity (s390)", "B663C618-E7BC-4D6D-90AA-11B756BB1797"},
	{ "Linux", "Root verity partition for dm-verity (RISC-V 64-bit)", "B6ED5582-440B-4209-B8DA-5FF7C419EA3D"},
	{ "SoftRAID", "SoftRAID_Status", "B6FA30DA-92D2-4A9A-96F1-871EC6486200"},
	{ "Linux", "Root partition (AArch64)", "B921B045-1DF0-41C3-AF44-4C6F280D3FAE"},
	{ "Linux", "/usr partition (RISC-V 32-bit)", "B933FB22-5C3F-4F91-AF90-E2BB0FA50702"},
	{ "SoftRAID", "SoftRAID_Cache", "BBBA6DF5-F46F-4A89-8F59-8765B2727503"},
	{ "Linux", "/boot, as an Extended Boot Loader (XBOOTLDR) partition", "BC13C2FF-59E6-4262-A352-B275FD6F7172"},
	{ "freedesktop.org OSes (Linux, etc.)", "Shared boot loader configuration", "BC13C2FF-59E6-4262-A352-B275FD6F7172"},
	{ "Android-IA", "Config", "BD59408B-4514-490D-BF12-9878D963F378"},
	{ "Linux", "/usr verity partition for dm-verity (64-bit PowerPC big-endian)", "BDB528A5-A259-475F-A87D-DA53FA736A07"},
	{ "Container Linux by CoreOS", "Root filesystem on RAID (coreos-root-raid)", "BE9067B9-EA49-4F15-B4F6-F36F8C9E1818"},
	{ "Linux", "/usr partition (RISC-V 64-bit)", "BEAEC34B-8442-439B-A40B-984381ED097D"},
	{ "No OS", "Lenovo boot partition", "BFBFAFE7-A34F-448A-9A5B-6213EB736C22"},
	{ "No OS", "EFI System partition", "C12A7328-F81F-11D2-BA4B-00A0C93EC93B"},
	{ "Fuchsia legacy partitions", "fuchsia-esp", "C12A7328-F81F-11D2-BA4B-00A0C93EC93B"},
	{ "Linux", "/usr verity partition for dm-verity (ARM 32-bit)", "C215D751-7BCD-4649-BE90-6627490A4C05"},
	{ "Linux", "/usr verity signature partition for dm-verity (AArch64)", "C23CE4FF-44BD-4B00-B2D4-B41B3419E02A"},
	{ "Linux", "Root partition (64-bit PowerPC little-endian)", "C31C45E6-3F39-412E-80FB-4809C4980599"},
	{ "Linux", "/usr verity signature partition for dm-verity (RISC-V 32-bit)", "C3836A13-3137-45BA-B583-B16C50FE5EB4"},
	{ "Linux", "Root partition (TILE-Gx)", "C50CDD70-3862-4CC3-90E1-809A8C93EE2C"},
	{ "Android-IA", "Vendor", "C5A0AEEC-13EA-11E5-A1B1-001E67CA0C3C"},
	{ "Linux", "Root verity signature partition for dm-verity (s390x)", "C80187A5-73A3-491A-901A-017C3FA953E9"},
	{ "Linux", "/usr verity signature partition for dm-verity (64-bit PowerPC little-endian)", "C8BFBD1E-268E-4521-8BBA-BF314C399557"},
	{ "Plan 9", "Plan 9 partition", "C91818F9-8025-47AF-89D2-F030D7000C2C"},
	{ "Linux", "Root verity signature partition for dm-verity (mipsel: 32-bit MIPS little-endian)", "C919CC1F-4456-4EFF-918C-F75E94525CA5"},
	{ "Container Linux by CoreOS", "OEM customizations (coreos-reserved)", "C95DC21A-DF0E-4340-8D7B-26CBFA9A03E0"},
	{ "Linux", "/usr partition (mips64el: 64-bit MIPS little-endian)", "C97C1F32-BA06-40B4-9F22-236061B08AA8"},
	{ "Linux", "LUKS partition", "CA7D7CCB-63ED-4C53-861C-1742536059CC"},
	{ "ChromeOS", "ChromeOS firmware", "CAB6E88E-ABF3-4102-A07A-D4BB9BE3C1D3"},
	{ "Ceph", "Multipath block", "CAFECAFE-8AE0-4982-BF9D-5A8D867AF560"},
	{ "Ceph", "dm-crypt LUKS block", "CAFECAFE-9B03-4F30-B4C6-35865CEFF106"},
	{ "Ceph", "dm-crypt block", "CAFECAFE-9B03-4F30-B4C6-5EC00CEFF106"},
	{ "Ceph", "Block", "CAFECAFE-9B03-4F30-B4C6-B4B80CEFF106"},
	{ "Linux", "/usr verity partition for dm-verity (RISC-V 32-bit)", "CB1EE4E3-8CD0-4136-A0A4-AA61A32E8730"},
	{ "Linux", "/usr partition (s390)", "CD0F869B-D0FB-4CA0-B141-9EA87CC78D66"},
	{ "QNX", "Power-safe (QNX6) file system", "CEF5A9AD-73BC-4601-89F3-CDEEEEE321A1"},
	{ "Linux", "Root verity partition for dm-verity (x86)", "D13C5D3B-B5D1-422A-B29F-9454FDC89D76"},
	{ "Linux", "Root verity partition for dm-verity (PA-RISC)", "D212A430-FBC5-49F9-A983-A7FEEF2B8D0E"},
	{ "Linux", "Root partition (ARC)", "D27F46ED-2919-4CB8-BD25-9531F3C16534"},
	{ "Linux", "/usr verity signature partition for dm-verity (RISC-V 64-bit)", "D2F9000A-7A18-453F-B5CD-4D32F77A7B32"},
	{ "No OS", "Intel Fast Flash (iFFS) partition (for Intel Rapid Start technology)", "D3BFE2DE-3DAF-11DF-BA40-E3A556D89593"},
	{ "Linux", "Root verity signature partition for dm-verity (Alpha)", "D46495B7-A053-414F-80F7-700C99921EF8"},
	{ "Linux", "Root verity signature partition for dm-verity (64-bit PowerPC little-endian)", "D4A236E7-E873-4C07-BF1D-BF6CF7F1C3C6"},
	{ "Open Network Install Environment (ONIE)", "Config", "D4E6E2CD-4469-46F3-B5CB-1BFF57AFC149"},
	{ "Linux", "Root verity partition for dm-verity (mipsel: 32-bit MIPS little-endian)", "D7D150D2-2A04-4A33-8F12-16651205FF7B"},
	{ "Linux", "/usr verity signature partition for dm-verity (ARM 32-bit)", "D7FF812F-37D1-4902-A810-D76BA57B975A"},
	{ "Fuchsia standard partitions", "Durable mutable encrypted system data", "D9FD4535-106C-4CEC-8D37-DFC020CA87CB"},
	{ "Linux", "/usr partition (PA-RISC)", "DC4A4480-6917-4262-A4EC-DB9384949F25"},
	{ "Android-IA", "Data", "DC76DDA9-5AC1-491C-AF42-A82591580C0D"},
	{ "Fuchsia legacy partitions", "Zircon boot image (slot A)", "DE30CC86-1F4A-4A31-93C4-66F147D33E05"},
	{ "Windows", "Windows Recovery Environment", "DE94BBA4-06D1-4D40-A16A-BFD50179D6AC"},
	{ "Linux", "Root verity partition for dm-verity (AArch64)", "DF3300CE-D69F-4C92-978C-9BFB0F38D820"},
	{ "Linux", "/usr verity partition for dm-verity (32-bit PowerPC)", "DF765D00-270E-49E5-BC75-F47BB2118B09"},
	{ "Linux", "/usr partition (Alpha)", "E18CF08C-33EC-4C0D-8246-C6C6FB3DA024"},
	{ "HP-UX", "Service partition", "E2A1E728-32E3-11D6-A682-7B03A0000000"},
	{ "Windows", "Microsoft Reserved Partition (MSR)", "E3C9E316-0B5C-4DB8-817D-F92DF00215AE"},
	{ "Linux", "/usr partition (LoongArch 64-bit)", "E611C702-575C-4CBE-9A46-434FA0BF7E3F"},
	{ "Linux", "Logical Volume Manager (LVM) partition", "E6D6D379-F507-44C2-A23C-238F2A3DF928"},
	{ "Windows", "Storage Spaces partition", "E75CAF8F-F680-4CEE-AFA3-B001E56EFC2D"},
	{ "Linux", "/usr verity signature partition for dm-verity (x86-64)", "E7BB33FB-06CF-4E81-8273-E543B413E2E2"},
	{ "Linux", "Root verity signature partition for dm-verity (IA-64)", "E98B36EE-32BA-4882-9B12-0CE14655F46A"},
	{ "Android-IA", "Persistent", "EBC597D0-2053-4B15-8B64-E0AAC75F4DB1"},
	{ "Windows", "Basic data partition", "EBD0A0A2-B9E5-4433-87C0-68B6B72699C7"},
	{ "Ceph", "Multipath block DB", "EC6D6385-E346-45DC-BE91-DA2A7C8B3261"},
	{ "Linux", "/usr verity partition for dm-verity (64-bit PowerPC little-endian)", "EE2B9983-21E8-4153-86D9-B6901A54D1CE"},
	{ "Android-IA", "Misc", "EF32A33B-A409-486C-9141-9FFB711F6266"},
	{ "Linux", "Root verity signature partition for dm-verity (RISC-V 64-bit)", "EFE0F087-EA8D-4469-821A-4C2A96A8386A"},
	{ "Linux", "/usr verity signature partition for dm-verity (mips64el: 64-bit MIPS little-endian)", "F2C2C7EE-ADCC-4351-B5C6-EE9816B66E16"},
	{ "Linux", "Root verity partition for dm-verity (LoongArch 64-bit)", "F3393B22-E9AF-4613-A948-9D3BFBD0C535"},
	{ "No OS", "Sony boot partition", "F4019732-066E-4E12-8273-346C5641494F"},
	{ "Linux", "/usr verity partition for dm-verity (LoongArch 64-bit)", "F46B2C26-59AE-48F0-9106-C50ED47F673D"},
	{ "Linux", "Root verity signature partition for dm-verity (64-bit PowerPC big-endian)", "F5E2C20C-45B2-4FFA-BCE9-2A60737E1AAF"},
	{ "Fuchsia standard partitions", "Factory-provisioned read-only system data", "F95D940E-CABA-4578-9B93-BB6C90F29D3E"},
	{ "SoftRAID", "SoftRAID_Volume", "FA709C7E-65B1-4593-BFD5-E71D61DE9B02"},
	{ "Ceph", "Lockbox for dm-crypt keys", "FB3AABF9-D25F-47CC-BF5E-721D1816496B"},
	{ "Linux", "Root verity partition for dm-verity (Alpha)", "FC56D9E9-E6E5-4C06-BE32-E74407CE09A5"},
	{ "Linux", "/usr verity partition for dm-verity (ARC)", "FCA0598C-D880-4591-8C16-4EDA05C7347C"},
	{ "ChromeOS", "ChromeOS kernel", "FE3A2A5D-4F32-41A7-B725-ACCC3285A309"},
	{ "Fuchsia standard partitions", "Bootloader (slot A/B/R)", "FE8A2634-5E2E-46BA-99E3-3A192091A350"},
	{0, 0, 0}
};


int is_protective_mbr(mbr * boot_record) {
	/* TODO verificar si el MBR es un MBR de proteccion */
	/* Retorna 1 si el boot record tiene una tabla de particiones
	con solo una partición definida, de tipo GPT Protective MBR (0xEE) */
	return 0;
}


int is_valid_gpt_header(gpt_header * hdr) {
	/* TODO retorna 1 si el encabezado es valido (verificar el valor del atributo signature)*/
	return 0;
}


char * guid_to_str(guid * buf) {

	unsigned char bytes[sizeof(guid)];
	//Copy the bytes from the GUID
	memcpy(&bytes, buf, sizeof(guid));

	//Size of buffer : sizeof GUID plus four "-" plus NULL
	size_t ptr_sz = (sizeof (guid) * 2) + 5;

	char * ptr = (char*)malloc(ptr_sz);
	memset(ptr, 0,  ptr_sz);


	snprintf(ptr, ptr_sz,
			"%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
			bytes[3],
			bytes[2],
			bytes[1],
			bytes[0],
			bytes[5],
			bytes[4],
			bytes[7],
			bytes[6],
			bytes[8],
			bytes[9],
			bytes[10],
			bytes[11],
			bytes[12],
			bytes[13],
			bytes[14],
			bytes[15]
			);

	return ptr;
}

char * gpt_decode_partition_name(char name[72]) {
	int i;
	char * ptr = (char * )malloc((sizeof(short) * 36) + 1);
	char * name_ptr = (char*)name;

	for (i = 0; i< 36; i++) {
		ptr[i] = name_ptr[i*2];
	}

	ptr[36] = 0;

	return ptr;
}


int is_null_descriptor(gpt_partition_descriptor * desc) {

	return 0;
}



const gpt_partition_type * get_gpt_partition_type(char * guid_str) {

	/* TODO retornar el tipo de particion de acuerdo con el GUID especificado */

	//Default: return first element of partition type array
	return &gpt_partition_types[0];
}



