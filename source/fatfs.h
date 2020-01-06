/**
 * 
 */
#pragma once

#include <cstdint>
#include <string>
#include <vector>

// FIXME: This can be a union with two nested structs to take into
// account the differences between the FAT12/16 and FAT32 versions
struct MSDOSBiosBlock
{
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sector_count;
    uint8_t number_of_fats;
    uint16_t root_entry_count;
    uint16_t total_sectors16;
    uint8_t media_type;
    uint16_t fat_size16;
    uint16_t sectors_per_track;
    uint16_t number_of_heads;
    uint32_t number_of_hidden_sectors;
    uint32_t total_sectors32;

    // These are FAT32 specific variables!
    uint32_t fat_size32;
    uint16_t ext_flags;
    uint16_t fs_version;
    uint32_t root_cluster;
    uint16_t fsinfo_ptr;
    uint16_t backup_boot_ptr;
    uint8_t reserved[12];
    uint8_t drive_number;
    uint8_t reserved1; // This name is to remain consistent with Fatgen103
    uint8_t boot_signature;
    uint32_t volume_id;
    char volume_label[11];
    char fs_type[8]; // This is _always_ "FAT32   "
} __attribute__((packed));

// Boot Sector (VBR) of a FAT volume
struct MSDOSBootRecord
{
    uint8_t jmp[3];
    char oem_name[8];
    struct MSDOSBiosBlock bpb;
    uint8_t filler[420];
    uint16_t boot_signature; // 0xAA55
} __attribute__((packed)) ;

struct LongFileNameEntry
{
    uint8_t entry_order;
    uint16_t lfn1[5];
    uint8_t attribute; // Always 0x0f
    uint8_t long_entry_type;
    uint8_t checksum;
    uint16_t lfn2[6];
    uint16_t always0;
    uint16_t lfn3[2];
} __attribute__((packed));

struct DirectoryEntry
{
    char name[11];
    uint8_t attributes;
    uint8_t nt_reserved;
    uint8_t millisecond_creation_time;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access_date;
    uint16_t first_cluster_hi;
    uint16_t write_time;
    uint16_t write_date;
    uint16_t first_cluster_lo;
    uint32_t size;
} __attribute__((packed));

class FATFileSystem
{
    // We can still read bad clusters, however we should NEVER allocate them!
    static constexpr uint32_t BAD_CLUSTER = 0x0ffffff7;

    // In Serenity, we use this value to signal and of a cluster, however
    // any value greater than this is the of of a cluster too!
    static constexpr uint32_t END_OF_CLUSTER = 0x0ffffff8;

    static constexpr uint32_t CLUSTER_MASK = 0x0fffffff;
public:
    explicit FATFileSystem(const char* fname);

    void initialize();

    uint16_t bytes_per_sector() const { return m_bytes_per_sector; }
    uint16_t sectors_per_cluster() const { return m_sectors_per_cluster; }

    // Helper functions. There's a few.....
    uint64_t fat_sector() const { return m_reserved_sectors; } // Sector where the FAT begins
    uint64_t sector_to_offset(uint32_t sector) const { return m_bytes_per_sector * sector; }
    uint64_t cluster_to_sector(uint32_t cluster) const { return root_cluster() + cluster * m_sectors_per_cluster - (2 * m_sectors_per_cluster); }
    uint64_t cluster_fat_sector(uint32_t cluster) const { return m_reserved_sectors + ((cluster * 4) / m_bytes_per_sector); }
    uint64_t cluster_fat_offset(uint32_t cluster) const { return (cluster * 4) % m_bytes_per_sector; }
    uint32_t root_cluster() const { return m_reserved_sectors + (m_number_of_fats * m_sectors_per_fat); }

    void seek(uint64_t) const;

    const std::vector<DirectoryEntry> read_directory(uint32_t cluster) const;

private:
    void print_bpb(const MSDOSBootRecord&) const;
    void read_sector(uint8_t*, uint64_t) const;
    void read_cluster(uint8_t*, uint32_t) const;

    const std::vector<uint32_t> cluster_chain(uint32_t) const;

private:
    std::string m_volume_name;

    FILE* m_volume;

    // Here's some important data values we pull from the BPB for quick lookup
    // without storing the whole struct
    uint16_t m_bytes_per_sector { 0 };
    uint16_t m_sectors_per_cluster { 0 };
    uint16_t m_root_cluster { 0 };
    uint16_t m_reserved_sectors { 0 };
    uint8_t m_number_of_fats { 0 };
    uint32_t m_sectors_per_fat { 0 };

    uint32_t m_current_directory { 0 }; // This is actually a cluster!
};