/**
 * 
 * 
 */
#include "fatfs.h"

#include <cstdio>

FATFileSystem::FATFileSystem(const char* fname) : m_volume_name(fname), m_volume()
{
    initialize();
}

void FATFileSystem::print_bpb(const MSDOSBootRecord& vbr)
{
    printf("===MSDOS BOOT RECORD DATA===\n");
    printf("jmp: \t\t\t0x%x 0x%x 0x%x\n", vbr.jmp[0], vbr.jmp[1], vbr.jmp[2]);
    printf("oem_name: \t\t%s\n", std::string(vbr.oem_name).c_str());
    printf("bytes per sector: \t%d\n", vbr.bpb.bytes_per_sector);
    printf("sectors per cluster: \t%d\n", vbr.bpb.sectors_per_cluster);
    printf("rsvd sector count: \t%d\n", vbr.bpb.reserved_sector_count);
    printf("number of fats: \t%d\n", vbr.bpb.number_of_fats);
    printf("root entry count: \t%d\n", vbr.bpb.root_entry_count);
    printf("total sectors16: \t%d\n", vbr.bpb.total_sectors16);
    printf("media type: \t\t%d\n", vbr.bpb.media_type);
    printf("fat size 16: \t\t%d\n", vbr.bpb.fat_size16);
    printf("sectors per track: \t%d\n", vbr.bpb.sectors_per_track);
    printf("num heads: \t\t%d\n", vbr.bpb.number_of_heads);
    printf("num hidden sectors: \t%d\n", vbr.bpb.number_of_hidden_sectors);
    printf("total sectors: \t\t%d\n", vbr.bpb.total_sectors32);
    printf("fat size \t\t%d\n", vbr.bpb.fat_size32);
    printf("flags: \t\t\t0x%x\n", vbr.bpb.ext_flags);
    printf("fs version: \t\t%d\n", vbr.bpb.fs_version);
    printf("root cluster \t\t%d\n", vbr.bpb.root_cluster);
    printf("fsinfo ptr: \t\t0x%x\n", vbr.bpb.fsinfo_ptr);
    printf("bk boot: \t\t0x%x\n", vbr.bpb.backup_boot_ptr);
    printf("drive num: \t\t0x%x\n", vbr.bpb.drive_number);
    printf("boot sig: \t\t0x%x\n", vbr.bpb.boot_signature);
    printf("volume id \t\t0x%x\n", vbr.bpb.volume_id);
    printf("volume label: \t\t%s\n", std::string(vbr.bpb.volume_label).c_str());
    printf("fs type: \t\t%s\n", std::string(vbr.bpb.fs_type).c_str()); // This is _always_ "FAT32   "
    printf("boot sig: \t\t0x%x\n", vbr.boot_signature);
}

void FATFileSystem::seek(uint64_t sector)
{
    fseek(m_volume, sector * m_bytes_per_sector, SEEK_SET);
}

void FATFileSystem::initialize()
{
    m_volume = fopen(m_volume_name.c_str(), "rb");
    if(!m_volume)
    {
        fprintf(stderr, "Unable to open volume %s!\n", m_volume_name.c_str());
        return;
    }

    MSDOSBootRecord vbr;
    fread(&vbr, sizeof(MSDOSBootRecord), 1, m_volume);
    print_bpb(vbr);
    m_bytes_per_sector = vbr.bpb.bytes_per_sector;
    m_sectors_per_cluster = vbr.bpb.sectors_per_cluster;
    m_root_cluster = vbr.bpb.root_cluster;
    m_reserved_sectors = vbr.bpb.reserved_sector_count;
    m_number_of_fats = vbr.bpb.number_of_fats;
    m_sectors_per_fat = vbr.bpb.fat_size32;
}