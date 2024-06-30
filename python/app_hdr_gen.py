''' 
    @author -->  himanshu 
    Date --> May 7, 2024
    
    @brief --> ota meta data binary file generator 
    
'''
import sys
import struct 
import json
import hashlib
import binascii

RED = '\033[91m'
GREEN = '\033[92m'
YELLOW = '\033[93m'
BLUE = '\033[94m'
MAGENTA = '\033[95m'
CYAN = '\033[96m'
RESET = '\033[0m'


# input bin file 
in_file = sys.argv[1]


# esp_app_descriptor_size = 256

''' 
================== Function defination here =========================================
 
    '''
    
''' get the sha hash of the file '''
def calculate_file_sha256(file_path, file_len):
    # Initialize the SHA-256 hash object
    sha256_hash = hashlib.sha256()
    
    def get_file_chunks():
        chunk_size = 1024;current_len=0
        # Open the file in binary mode
        with open(file_path, "rb") as f:
            while current_len < file_len:
                # read the min of remaining len and chunk size 
                # Read the next chunk (but no more than remaining bytes)
                chunk = f.read(min(chunk_size, (file_len-current_len)))
                if not chunk:
                    break
                # use yield for generators instead of return 
                yield chunk 
                current_len += len(chunk)
            
            # Read the file in chunks to handle large files
            # for byte_block in iter(lambda: f.read(4096), b""):
        
    for data in get_file_chunks():
        sha256_hash.update(data)
    
    # Return the hexadecimal digest of the hash
    return sha256_hash.hexdigest() 

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

# typedef struct {
#     uint32_t magic_word;        /*!< Magic word ESP_APP_DESC_MAGIC_WORD */
#     uint32_t secure_version;    /*!< Secure version */
#     uint32_t reserv1[2];        /*!< reserv1 */
#     char version[32];           /*!< Application version */
#     char project_name[32];      /*!< Project name */
#     char time[16];              /*!< Compile time */
#     char date[16];              /*!< Compile date*/
#     char idf_ver[32];           /*!< Version IDF */
#     uint8_t app_elf_sha256[32]; /*!< sha256 of elf file */
#     uint32_t reserv2[20];       /*!< reserv2 */
# } esp_app_desc_t;


# /// @brief app descriptor structure
# typedef struct __ESP_APP_CUSTOM_DESCP__
# {
#     uint32_t magic_number;
#     uint16_t size; // size would only be until app_extra_size

#     /// @brief  the app descriptor header version
#     ota_header_version_t hdr_ver;
#     /// @brief app version
#     ota_header_version_t app_ver;
#     /// @brief app name is null terminated
#     const char app_name[APP_NAME_LEN];
#     // describe the app type, DFU, APP
#     uint8_t app_type;
#     uint32_t app_size;

#     /// @brief app verification state same as app state in ota meta data
#     uint32_t app_verif_state;
#     /// @brief booting state of the app same as boot index
#     uint32_t app_boot_state;

#     // this contains the sizeof(app_rfu) + sizeof(app_crash) == (sizeof(anyone)/2)
#     uint32_t app_extra_size;

#     uint8_t extra_mem[0]; // flexible array member
# } PACKED esp_app_custom_desc_t;

# Define the structure format -===== esp_image_header + esp_segmnet_header + esp_custom_descriptor --> esp_descriptor 
ESP_IMAGE_HEADER_STRUCT_FORMAT = '<BBBB IB 3B H B HH 4BB'
ESP_IMAGE_SEGMENT_HEADER_STRUCT_FORMAT = '<II'
ESP_APP_DESCRIPTOR_STRUCT_FORMAT = '<I I 2I 32b 32b 16b 16b 32b 32B 20I'
ESP_APP_CUSTOM_DESC_STRUCT_FORMAT = '<I H 4B 4B 32b 32B I II I'

ESP_APP_CUSTOM_DESC_SIZE_INDEX = '<I H 4B 4B 32b 32B'

APP_CUSTOM_DESCRIPTOR_APP_SIZE = '<I'


'''calculate the image length of the binary '''
def calculate_image_len():    
    global in_file
    global ESP_IMAGE_HEADER_STRUCT_FORMAT,ESP_IMAGE_SEGMENT_HEADER_STRUCT_FORMAT
    global ESP_APP_DESCRIPTOR_STRUCT_FORMAT,ESP_APP_CUSTOM_DESC_SIZE_INDEX
    
    esp_image_hdr =0

    with open(in_file,"rb") as f:
        f.seek(0)
        print(f"{GREEN} reading size {struct.calcsize(ESP_IMAGE_HEADER_STRUCT_FORMAT)} from {in_file} {RESET}")
        esp_image_hdr_struct_bytes = f.read(struct.calcsize(ESP_IMAGE_HEADER_STRUCT_FORMAT))
        esp_image_hdr = struct.unpack(ESP_IMAGE_HEADER_STRUCT_FORMAT, esp_image_hdr_struct_bytes)

    segment_count = esp_image_hdr[1]

    print(f"{GREEN} the magic no is {esp_image_hdr[0]} and segment count is {esp_image_hdr[1]} {RESET}") 


    segment_offset = struct.calcsize(ESP_IMAGE_HEADER_STRUCT_FORMAT)

    # typedef struct {
    #     uint32_t load_addr;     /*!< Address of segment */
    #     uint32_t data_len;      /*!< Length of data */
    # } esp_image_segment_header_t;


    # read the offset of the segment and find where the checksum and hash are present 
    for x in range(segment_count):
        # read the segment offset 
        with open(in_file , "rb") as f: 
            f.seek(segment_offset)
            # read the segment header structure from the binary 
            segment_data_struct = f.read(struct.calcsize(ESP_IMAGE_SEGMENT_HEADER_STRUCT_FORMAT))
            segment_data =struct.unpack(ESP_IMAGE_SEGMENT_HEADER_STRUCT_FORMAT,segment_data_struct)
            # segment data length is presen at first index , also add the segment header size (also contribute to size)
            segment_offset += segment_data[1] + struct.calcsize(ESP_IMAGE_SEGMENT_HEADER_STRUCT_FORMAT)
            print(f"the segment is {x} and len is {hex(segment_data[1]).zfill(8)}, Load {hex(segment_data[0]).zfill(8)}  ")
            
    # /// return the segment count and segment offset 
    return segment_count,segment_offset


def bin_write_app_desc_custom_data(app_data:dict):
    global in_file
    global ESP_IMAGE_HEADER_STRUCT_FORMAT,ESP_IMAGE_SEGMENT_HEADER_STRUCT_FORMAT
    global ESP_APP_DESCRIPTOR_STRUCT_FORMAT,ESP_APP_CUSTOM_DESC_SIZE_INDEX
    # Pack the data into binary format
    # packed_data = struct.pack(struct_format, *data)
    # packed_data = packed_data.ljust(100, b'\x00')  # Ensure it's 100 bytes long
    app_hdr_data = (str(app_data_json) + "\0").encode('utf-8')
    print(app_hdr_data)
     
    # calculated by estmating the size of the image_hdr + segment_hdr + app_descriptor 
    offset = struct.calcsize(ESP_IMAGE_HEADER_STRUCT_FORMAT) +  \
            struct.calcsize(ESP_IMAGE_SEGMENT_HEADER_STRUCT_FORMAT) +   \
            struct.calcsize(ESP_APP_DESCRIPTOR_STRUCT_FORMAT) + \
            struct.calcsize(ESP_APP_CUSTOM_DESC_STRUCT_FORMAT)
            
    print(f"{RED} modifying the header of {in_file} at offset {offset}  and len to write {len(app_hdr_data)} {RESET}")

    # Write the binary data to a file
    with open(in_file, 'r+b') as f:
        f.seek(offset)
        f.write(app_hdr_data)



''' write and read the app size to the custom descriptor '''
def write_and_read_app_size(image_len: int ):
    global in_file
    global ESP_IMAGE_HEADER_STRUCT_FORMAT,ESP_IMAGE_SEGMENT_HEADER_STRUCT_FORMAT
    global ESP_APP_DESCRIPTOR_STRUCT_FORMAT,ESP_APP_CUSTOM_DESC_SIZE_INDEX
    
    offset = struct.calcsize(ESP_IMAGE_HEADER_STRUCT_FORMAT) +  \
            struct.calcsize(ESP_IMAGE_SEGMENT_HEADER_STRUCT_FORMAT) +   \
            struct.calcsize(ESP_APP_DESCRIPTOR_STRUCT_FORMAT) + \
            struct.calcsize(ESP_APP_CUSTOM_DESC_SIZE_INDEX)
    
    print(f"{GREEN} writing the image length of {image_len} to the offset {offset}  to {in_file} {RESET}")
    with open(in_file,"r+b")as f:
        # mmove to that position and write the size 
        f.seek(offset)
        f.write(struct.pack(APP_CUSTOM_DESCRIPTOR_APP_SIZE, image_len))
    
    with open(in_file, "rb")as f:
        f.seek(offset)
        app_size_bytes = f.read(struct.calcsize(APP_CUSTOM_DESCRIPTOR_APP_SIZE))
        app_size = struct.unpack(APP_CUSTOM_DESCRIPTOR_APP_SIZE, app_size_bytes)
        print(f"the app size from the bin is {app_size[0]}") 
    
    
    

def write_and_read_image_hash(image_len):
    global in_file
    # since cehksum is present at a 16 byte boundary , we have to find the offset with respect to last address 
    padded_byte_boundary = 16-(image_len%16)
    hash_struct_format = "<32B"
    
    bin_image_sha =  calculate_file_sha256(in_file,image_len)
    # .write own sha hash on the image 
    # calculate the sha and show to user 
    print(f"the SHA of the file is {bin_image_sha}")
        
    # write the new hash to the file 
    with open(in_file,"r+b")as f:
        f.seek(image_len + padded_byte_boundary)
        # unhexify the sha string to hex bytes an write to the vlaue
        f.write(binascii.unhexlify(bin_image_sha))
        
    # -================ read the image sha from the file ============================ 
    with open(in_file,"rb") as f:
        f.seek(image_len + padded_byte_boundary)
        in_data_bytes = f.read(struct.calcsize(hash_struct_format))
        hash_data =  struct.unpack(hash_struct_format, in_data_bytes) 
        hash_str = ''.join(hex(ele).removeprefix('0x').zfill(2) for ele in hash_data)
        print(f"the SHA raead from the file {hash_str}")
    


app_data_json = {"Hardware ver": "234SD" ,
    "Serial num": "35861206023","Firmware ver": "V1.2.2",
    "Device num": "XXXXXX", "Manuf. name": "Marbles.Health"}



bin_write_app_desc_custom_data(app_data_json)
segment_count,image_len = calculate_image_len()

print(f"{RED} the segment count {segment_count} and image len is {image_len}  {RESET}")
# read and write the image size 
write_and_read_app_size(image_len)
write_and_read_image_hash(image_len)


