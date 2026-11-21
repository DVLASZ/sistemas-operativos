/**
* @file
* @brief Definiciones para discos inicializados con esquema MBR
* @author Erwin Meza Vega <emezav@unicauca.edu.co>
* @author Edward Esteban Dávila Salazar
* @author Miguel Angel Perez Mera
*/

#ifndef MBR_H
#define MBR_H

/** @brief Boot sector signature */
#define MBR_SIGNATURE 0xAA55

/** @brief GPT Partition type - MBR */
#define MBR_TYPE_GPT 0xEE

/** @brief Unused partition table - MBR*/
#define MBR_TYPE_UNUSED 0x00

/** @brief Longitud maxima del texto para el tipo de particion */
#define TYPE_NAME_LEN 256

/** @brief Tamano de un sector en bytes */
#define SECTOR_SIZE 512

/** @brief Cantidad de entradas en la tabla de particiones de un MBR */
#define MBR_PARTITIONS 4

/** @brief Indicador de arranque: particion activa */
#define MBR_STATUS_ACTIVE 0x80

/** @brief Partition descriptor - MBR */
typedef struct {
	unsigned char status; /*!< Indicador de arranque (0x80 = activa) */
	unsigned char first_chs[3]; /*!< Sector inicial en formato C/H/S */
	unsigned char type; /*!< Tipo de particion */
	unsigned char last_chs[3]; /*!< Sector final en formato C/H/S */
	unsigned int first_lba; /*!< Sector inicial (LBA) */
	unsigned int sectors; /*!< Tamano de la particion en sectores */
}__attribute__((packed)) mbr_partition_descriptor;

/** @brief Master Boot Record. */
typedef struct {
	unsigned char code[446]; /*!< Codigo de arranque */
	mbr_partition_descriptor partitions[MBR_PARTITIONS]; /*!< Tabla de particiones */
	unsigned short signature; /*!< Firma, 0xAA55 */
}__attribute__((packed)) mbr;

/**
* @brief Checks if a bootsector is a MBR.
* @param boot_record Bootsector read in memory]
* @return 1 If the bootsector is a MBR, 0 otherwise.
*/
int is_mbr(mbr * boot_record);

/**
* @brief Text description of a MBR partition type
* @param type Partition type reported in MBR
* @param buf String buffer to store the text description
*/
void mbr_partition_type(unsigned char type, char buf[TYPE_NAME_LEN]);

/**
* @brief Dice si un tipo de particion MBR es extendida (0x05, 0x0F o 0x85)
* @param type Tipo de particion
* @return 1 si es extendida, 0 si no
*/
int mbr_is_extended(unsigned char type);

/**
* @brief Cilindro de un campo C/H/S de tres bytes
*
* Los dos bits altos del segundo byte son los dos bits altos del cilindro (10 bits en total).
*
* @param chs Los tres bytes tal como estan en el descriptor
* @return Cilindro, de 0 a 1023
*/
unsigned int mbr_chs_cylinder(const unsigned char chs[3]);

/**
* @brief Cabeza de un campo C/H/S de tres bytes
* @param chs Los tres bytes tal como estan en el descriptor
* @return Cabeza, de 0 a 255
*/
unsigned int mbr_chs_head(const unsigned char chs[3]);

/**
* @brief Sector de un campo C/H/S de tres bytes (los seis bits bajos del segundo byte)
* @param chs Los tres bytes tal como estan en el descriptor
* @return Sector, de 1 a 63
*/
unsigned int mbr_chs_sector(const unsigned char chs[3]);

/**
* @brief Convierte un C/H/S a LBA con 16 cabezas y 63 sectores por pista
*
* LBA = (c * 16 * 63) + (h * 63) + (s - 1)
*
* Es la formula del enunciado. Los programas de particionado de hoy llenan el C/H/S con 255 cabezas,
* asi que para un cilindro mayor que 0 el resultado puede no coincidir con el LBA real.
*
* @param chs Los tres bytes tal como estan en el descriptor
* @return LBA equivalente
*/
unsigned long long mbr_chs_to_lba(const unsigned char chs[3]);

/**
* @brief Dice si el C/H/S es la marca FE FF FF, que obliga a usar el LBA
* @param chs Los tres bytes tal como estan en el descriptor
* @return 1 si es la marca, 0 si no
*/
int mbr_chs_is_lba_marker(const unsigned char chs[3]);

#endif
