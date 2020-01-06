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

void FATFileSystem::print_bpb(const MSDOSBootRecord& vbr) const
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

void FATFileSystem::seek(uint64_t sector) const
{
    printf("seeking to sector %d(0x%x)\n", sector, sector * 512);
    fseek(m_volume, sector * m_bytes_per_sector, SEEK_SET);
}

void FATFileSystem::read_sector(uint8_t* buffer, uint64_t sector) const
{
    printf("reading sector %d(0x%x)\n", sector, sector * 512);
    seek(sector);
    fread(buffer, m_bytes_per_sector, 1, m_volume);
}

void FATFileSystem::read_cluster(uint8_t* buffer, uint32_t cluster) const
{
    printf("reading cluster %d (starting at sector %d)\n", cluster, cluster * m_sectors_per_cluster);
    seek(cluster_to_sector(cluster));
    fread(buffer, m_bytes_per_sector * m_sectors_per_cluster, 1, m_volume);
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

const std::vector<uint32_t> FATFileSystem::cluster_chain(uint32_t start_cluster) const
{
    std::vector<uint32_t> ret;
    uint8_t buffer[m_bytes_per_sector];
    uint32_t cluster_value = start_cluster;

    ret.push_back(start_cluster);
    do
    {
        // Let's get the corresponding FAT sector and offset
        uint64_t sector = cluster_fat_sector(cluster_value);
        uint64_t offset = cluster_fat_offset(cluster_value);

        read_sector(buffer, sector);
        cluster_value = *(reinterpret_cast<uint32_t*>(&buffer[offset]));

        if(cluster_value == BAD_CLUSTER)
            printf("warning: bad cluster!\n");

        if(cluster_value <= BAD_CLUSTER)
            ret.push_back(cluster_value & CLUSTER_MASK);

        printf("cluster value 0x%x\n", cluster_value);

    } while ((cluster_value & CLUSTER_MASK) <= BAD_CLUSTER);
    
    printf("done! Number of clusters is %d\n", ret.size());
    return ret;
}

const std::vector<DirectoryEntry> FATFileSystem::read_directory(uint32_t cluster) const
{
    std::vector<DirectoryEntry> ret;
    std::vector<uint32_t> clusters = cluster_chain(cluster);

    std::vector<uint8_t> buf;
    buf.resize(clusters.size() * m_sectors_per_cluster * m_bytes_per_sector);

    int sector_offset = 0;
    for(uint32_t cluster : clusters)
    {
        read_cluster(&buf.data()[sector_offset * m_sectors_per_cluster * m_bytes_per_sector], cluster);
        sector_offset++;
    }

    // Alright! We now have all of the data in the directory, now we can
    // parse all of the data
    int directory_byte_offset = 0;
    for(;;) 
    {
        std::string fname;
        int num_to_read =  buf.data()[directory_byte_offset] & 0x0f;

        for(int i = 0; i < num_to_read; i++)
        {
            LongFileNameEntry* lfn = reinterpret_cast<LongFileNameEntry*>(&buf.data()[directory_byte_offset]);

            for(int j = 0; j < 13; j++)
            {
                if(j < 5 && lfn->lfn1[j] != 0xffff)
                    fname += static_cast<char>(lfn->lfn1[j] & 0xff);
                else if(j < 11 && lfn->lfn2[j - 5] != 0xffff)
                    fname += static_cast<char>(lfn->lfn2[j - 5] & 0xff);
                else if(j < 13 && lfn->lfn3[j - 11] != 0xffff)
                    fname += static_cast<char>(lfn->lfn3[j - 11] & 0xff);
            }
            
            directory_byte_offset += sizeof(LongFileNameEntry);
        }

        printf("%s\n", fname.c_str());
        if(num_to_read == 0)
            break; // Out of dirents to read!

        //directory_byte_offset += sizeof(LongFileNameEntry) * num_to_read;
        DirectoryEntry* dirent = reinterpret_cast<DirectoryEntry*>(&buf.data()[directory_byte_offset]);
        ret.push_back(*dirent);
        directory_byte_offset += sizeof(DirectoryEntry);
    }

    return ret;
}