# BootLoader For Cortex M4 Processor 
BootLoader for STM32F407VGTX 

## What API'S this Repo provide ?   

### Get_Version :
#### Return the current Version of BL (Major.Minor.Patch).   
### Get_Help
#### Provide all the commands available to choose from.
### Get_Chip_Identification_Number   
#### Get Identification Number of MC used.
### Read_Protection_Level 
#### Get Flash protection Level (Level0,1,2).
### Jump_To_Address       
#### Jump to Certain Address with flash or Ram .
### Erase_Flash  
#### Erase flash and it have 2 Options Mass Erase or Sector Erase.
### Change_Read_Protection
#### Change Flash protection level .
### Memory_Write   
#### To load your hex file and burn it on your MC.
