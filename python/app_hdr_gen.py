''' 
    @author -->  himanshu 
    Date --> May 7, 2024
    
    @brief --> ota meta data binary file generator 
    
'''
import sys
import struct 
import json


RED = '\033[91m'
GREEN = '\033[92m'
YELLOW = '\033[93m'
BLUE = '\033[94m'
MAGENTA = '\033[95m'
CYAN = '\033[96m'
RESET = '\033[0m'


# input bin file 
in_file = sys.argv[1]

# Pack the data into binary format
# packed_data = struct.pack(struct_format, *data)
packed_data = "this is to be placed"
packed_data = packed_data.encode('utf-8')
# packed_data = packed_data.ljust(100, b'\x00')  # Ensure it's 100 bytes long

"Hardware version" : "X4023" 

position = 351
print(f"{RED} modifying the header of {in_file}  {RESET}")

# Write the binary data to a file
with open(in_file, 'r+b') as f:
    f.seek(position)
    f.write(packed_data)


# after writing the data we also have to correct the checksum and hash appended of the binary 

uint32_max = 0xFFFFFFFF
uint8_t_max = 0xFF


INT_SIZE = 4
# esp_app_descriptor_size = 256
 

# uint8_t magic;              /*!< Magic word ESP_IMAGE_HEADER_MAGIC */
# uint8_t segment_count;      /*!< Count of memory segments */
# uint8_t spi_mode;           /*!< flash read mode (esp_image_spi_mode_t as uint8_t) */
# uint8_t spi_speed: 4;       /*!< flash frequency (esp_image_spi_freq_t as uint8_t) */
# uint8_t spi_size: 4;        /*!< flash chip size (esp_image_flash_size_t as uint8_t) */
# uint32_t entry_addr;        /*!< Entry address */
# uint8_t wp_pin;            /*!< WP pin when SPI pins set via efuse (read by ROM bootloader,
#                                 * the IDF bootloader uses software to configure the WP
#                                 * pin and sets this field to 0xEE=disabled) */
# uint8_t spi_pin_drv[3];     /*!< Drive settings for the SPI flash pins (read by ROM bootloader) */
# esp_chip_id_t chip_id;      /*!< Chip identification number */
# uint8_t min_chip_rev;       /*!< Minimal chip revision supported by image
#                                  * After the Major and Minor revision eFuses were introduced into the chips, this field is no longer used.
#                                  * But for compatibility reasons, we keep this field and the data in it.
#                                  * Use min_chip_rev_full instead.
#                                  * The software interprets this as a Major version for most of the chips and as a Minor version for the ESP32-C3.
#                                  */
# uint16_t min_chip_rev_full; /*!< Minimal chip revision supported by image, in format: major * 100 + minor */
# uint16_t max_chip_rev_full; /*!< Maximal chip revision supported by image, in format: major * 100 + minor */
# uint8_t reserved[4];        /*!< Reserved bytes in additional header space, currently unused */
# uint8_t hash_appended;      /*!< If 1, a SHA256 digest "simple hash" (of the entire image) is appended after the checksum.
#                                  * Included in image length. This digest
#                                  * is separate to secure boot and only used for detecting corruption.
#                                  * For secure boot signed images, the signature
#                                  * is appended after this (and the simple hash is included in the signed data). */

# Define the structure format
ESP_IMAGE_HEADER_STRUCT_FORMAT = '<BBBB IB3BHB HH4BB'

esp_image_hdr_struct =0

with open(in_file,"rb") as f:
    f.seek(0)
    print(f"{GREEN} reading the size from struct {struct.calcsize(ESP_IMAGE_HEADER_STRUCT_FORMAT)} from {in_file} {RESET}")
    esp_image_hdr_struct = f.read(struct.calcsize(ESP_IMAGE_HEADER_STRUCT_FORMAT))
    
esp_image_hdr = struct.unpack(ESP_IMAGE_HEADER_STRUCT_FORMAT, esp_image_hdr_struct)

ESP_SEGMENT_COUNT = esp_image_hdr[1]

print(f"{GREEN} the magic no is {esp_image_hdr[0]} and segment count is {esp_image_hdr[1]} {RESET}") 


# ===========================================================================
# get the total no of segment data length

segment_offset = struct.calcsize(ESP_IMAGE_HEADER_STRUCT_FORMAT)

# typedef struct {
#     uint32_t load_addr;     /*!< Address of segment */
#     uint32_t data_len;      /*!< Length of data */
# } esp_image_segment_header_t;

ESP_IMAGE_SEGMENT_HEADER_STRUCT_FORMAT = '<II'

# read the offset of the segment and find where the checksum and hash are present 
for x in range(ESP_SEGMENT_COUNT):
    # read the segment offset 
    with open(in_file , "rb") as f: 
        f.seek(segment_offset)
        segment_data_struct = f.read(struct.calcsize(ESP_IMAGE_SEGMENT_HEADER_STRUCT_FORMAT))
        segment_data =struct.unpack(ESP_IMAGE_SEGMENT_HEADER_STRUCT_FORMAT,segment_data_struct)
        # segment data length is presen at first index 
        segment_offset += segment_data[1] + struct.calcsize(ESP_IMAGE_SEGMENT_HEADER_STRUCT_FORMAT)
        print(f"the segment is {x} and len is {hex(segment_data[1])}, offset {hex(segment_data[0])} ")
        
# the segment offset is 
print(f"{RED} the total segment len is {hex(segment_offset)} {RESET}")

# since cehksum is present at a 16 byte boundary , we have to find the offset with respect to last address 
padded_byte_boundary = 16-(segment_offset%16)
hash_struct_format = f"<{padded_byte_boundary}B32B"

# now read 50 bytes from here 
with open(in_file,"rb")as f:
    f.seek(segment_offset)
    in_data_bytes = f.read(struct.calcsize(hash_struct_format))
    in_struct =  struct.unpack(hash_struct_format, in_data_bytes) 
    #  Convert the tuple of bytes to a bytes object
    checksum = ''.join(hex(ele).removeprefix('0x').zfill(2) for ele in in_struct[:padded_byte_boundary]) 
    hash_struct = [hex(x) for x in in_struct[padded_byte_boundary:]]
    hash_str = ''.join(ele.removeprefix('0x').zfill(2) for ele in hash_struct)
    print(f"checkusm = {checksum}")
    print(hash_str)
    

print("================================================")