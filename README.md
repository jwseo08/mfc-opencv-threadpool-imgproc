# OpenCV MFC Image Processing Application

Windows MFC 기반의 영상 처리 데모 애플리케이션입니다.

## Features

- OpenCV 기반 영상 처리
- CLAHE 기반 부분 영역 대비 향상
- 멀티스레드 처리
- Thread Pool 기반 작업 처리
- Sequential / Multi-thread 성능 비교
- MFC 메시지 기반 UI 연동

## Environment

- Windows 11
- Visual Studio 2022
- C++17
- OpenCV 4.x

## Screenshots

프로그램 실행 화면

## Architecture

- UI Thread
- Worker Thread Pool
- Single Task Worker
- Message-based UI Notification