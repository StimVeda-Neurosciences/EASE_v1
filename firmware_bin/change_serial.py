# import os

# serial_no_file = None


# # read the index and start programming 
# csv_file_path = "serial_no.txt"
# bin_file_path = "test.bin"

# # open the python file 
# bin_file = open(bin_file_path, "w")
# with open(csv_file_path, "r") as csv_file:
#     data = csv_file.read()


# line_no = input("enter the Serial number to be program ")

# split_data =data.split("\n")

# if (len(split_data) < int(line_no)):
#     print("Error: list length is less than " , line_no)
#     exit(2)

# elif (int(line_no) ==0):
#     print("Error: Serial number cant be zero ")
#     exit(3)
    
# else :
#     print("the Serial number that is gonna Programmed is ->",split_data[int(line_no)-1])
#     bin_file.write(split_data[int(line_no)-1])
#     # program the null for string terminator in the file 
#     bin_file.write("\0")


# bin_file.close()
# csv_file.close()

import os

serial_no_file = None


# read the index and start programming 
# csv_file_path = "serial_no.txt"
bin_file_path = "serial_number.bin"

# open the python file 
bin_file = open(bin_file_path, "w")

serial_number = input("enter the PCB Serial number to be programmed Format-> WWYYssss \r\n ")
pcb_number = input("enter the PCB Hardware number to be programmed Foramt -> xxyy \r\n ")


# if ((int(serial_number) ==0) or (int(pcb_number ==0))):
#     print("Error: Serial number or PCB number cant be zero ")
#     exit(2)
    
# else :
print("the Serial number that is gonna Programmed is shown below")
# text_to_write = "EA-v1.1.0."+ pcb_number +",242024-EAV1-"+serial_number
text_to_write = pcb_number+","+serial_number+" "
print(text_to_write)
bin_file.write(text_to_write)
# program the null for string terminator in the file 
bin_file.write("\0")


bin_file.close()