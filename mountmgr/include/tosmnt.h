
#ifndef __TOSMNT_H__
#define __TOSMNT_H__


/*
 * struct tosmnt_info
 * 마운트 정보를 담고 있는 구조체
 *
 * dev_name: 디바이스 파일 이름 (ex. "ada0p2", "da0s1c", ...)
 * fs_name: 디바이스의 파일시스템 이름 (ex. "ufs", "fat32", ...)
 * drive_letter: 디바이스에 부여된 드라이브 문자
 */

struct tosmnt_info
{
    char dev_name[0x10];
    char fs_name[0x10];
    char drive_letter;
};


#ifdef __cplusplus
extern "C" {
#endif


/*
 * tosmnt_query_drive
 * 주어진 드라이브 문자에 대한 마운트 정보를 조회
 *
 * Mount Manager로부터 마운트 정보를 얻어와서 info에 채운다.
 * info를 NULL로 하여 드라이브 문자가 할당되었는지 확인만 하는 용도로
 * 사용할 수도 있다.
 *
 * Return values:
 * 성공하면 info에 마운트 정보를 채운 뒤 0을 리턴한다.
 * drive_letter가 할당되지 않은 경우에는 1을 리턴하고, 다른 이유로
 * 실패하면 -1을 리턴한다.
 */

int tosmnt_query_drive(char drive_letter, struct tosmnt_info *info);


/*
 * tosmnt_unmount_drive
 * 주어진 드라이브 문자에 마운트 된 디바이스를 언마운트
 *
 * Mount Manager에게 주어진 드라이브 문자에 마운트 된 디바이스를
 * 언마운트하도록 요청한다. 디바이스가 언마운트되면 할당되었던 드라이브
 * 문자가 해제되며 관련된 자원들이 모두 정리된다.
 *
 * Return values:
 * 성공하면 0을 리턴한다. drive_letter가 할당되지 않았다면 1을 리턴하며,
 * 다른 이유로 실패하면 -1을 리턴한다.
 */

int tosmnt_unmount_drive(char drive_letter);


#ifdef __cplusplus
}
#endif


#endif /* __TOSMNT_H__ */

