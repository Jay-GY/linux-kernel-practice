#include <sys/inotify.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <iostream>


int main()
{
    // inotify 초기화
    // IN_NONBLOCK 플래그를 사용하여 비동기 모드로 설정
    // 이 플래그는 이벤트가 없을 때 read() 호출이 블록되지 않도록 함
    int fd = inotify_init1(IN_NONBLOCK);
    if (fd < 0) 
    {
        perror("inotify_init1 failed");
        std::cout << "errno = " << errno << " (" << strerror(errno) << ")" << std::endl;
        return 0;
    }

    // /tmp 디렉토리에 대한 감시 추가
    // IN_CREATE와 IN_DELETE 이벤트를 감시
    // IN_CREATE: 파일이 생성될 때
    // IN_DELETE: 파일이 삭제될 때
    // IN_NONBLOCK 플래그를 사용하여 이벤트가 없을 때 read() 호출
    int wd = inotify_add_watch(fd, "/tmp", IN_CREATE | IN_DELETE);
    if (wd == -1) 
    {
        perror("watch failed");
        std::cout << "errno = " << errno << " (" << strerror(errno) << ")" << std::endl;
        return 0;
    }

    // 이벤트를 읽기 위한 버퍼
    // inotify 이벤트는 가변 길이이므로 충분한 크기의 버퍼를 할당
    char buffer[1024];

    // 이벤트를 지속적으로 읽기
    while (1) 
    {
        // read() 호출로 이벤트를 읽음
        // IN_NONBLOCK 플래그가 설정되어 있으므로 이벤트가 없을 때 블록되지 않음
        // 이벤트가 없으면 EAGAIN 오류가 발생
        ssize_t len = read(fd, buffer, sizeof(buffer));
        if (len < 0) 
        {
            if (errno == EAGAIN) 
            {
                // EAGAIN 오류는 이벤트가 없음을 나타냄
                // 이 경우 잠시 대기 후 다시 시도
                // 10ms 대기
                usleep(10000);
                continue;
            } 
            else 
            {
                perror("read failed");
                std::cout << "errno = " << errno << " (" << strerror(errno) << ")" << std::endl;
                break;
            }
        }

        // 이벤트가 발생했을 때 처리
        // inotify_event 구조체를 사용하여 이벤트 정보를 파싱
        // 이벤트가 발생한 파일의 이름을 출력
        for (char *ptr = buffer; ptr < buffer + len;) 
        {
            // inotify_event 구조체를 포인터로 변환
            // 이벤트의 크기는 구조체의 크기와 이벤트 이름의 길이에 따라 다름
            // ptr는 현재 이벤트의 시작 위치를 가리킴
            struct inotify_event *event = (struct inotify_event *)ptr;

            // IN_CREATE: 파일이 생성되었을 때
            if (event->mask & IN_CREATE) 
            {
                printf("File created: %s\n", event->name);
            }
            // IN_DELETE: 파일이 삭제되었을 때 
            else if (event->mask & IN_DELETE) 
            {
                printf("File deleted: %s\n", event->name);
            }
            // 다음 이벤트로 이동
            ptr += sizeof(struct inotify_event) + event->len;
        }
    }

    // 감시 제거
    int result = inotify_rm_watch(fd, wd);
    if (result == -1) 
    {
        perror("inotify_rm_watch failed");
        std::cout << "errno = " << errno << " (" << strerror(errno) << ")" << std::endl;
    }

    // inotify 파일 디스크립터 닫기
    close(fd);

    return 0;

}