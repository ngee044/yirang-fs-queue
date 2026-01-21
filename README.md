# Yi-Rang MQ

**레거시 임베디드 환경을 위한 경량 메시지 큐**

[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)

---

## 왜 Yi-Rang MQ인가?

> "RabbitMQ는 너무 무겁고, 직접 만든 IPC는 관리가 안 된다"

레거시 임베디드 시스템에서 프로세스가 늘어날수록 IPC 통신은 복잡해집니다.
**Yi-Rang MQ**는 이런 환경을 위해 설계되었습니다:

| 문제 | Yi-Rang MQ 솔루션 |
|------|-------------------|
| 네트워크 스택 없음 | **파일 기반 Mailbox IPC** - TCP/IP 불필요 |
| 저사양 디바이스 | **SQLite/FileSystem** - 최소 의존성 |
| 전원 차단/재시작 | **Lease + Retry + DLQ** - 메시지 유실 방지 |
| 복잡한 브로커 설정 | **JSON 설정 파일 하나** - 즉시 시작 |
| 현장 디버깅 어려움 | **파일로 메시지 확인** - ls로 상태 파악 |

```
┌─────────────┐     File-based IPC      ┌─────────────┐
│  Producer   │ ──────────────────────> │   MainMQ    │
│  Process    │ <────────────────────── │   Daemon    │
└─────────────┘     (No Network!)       └─────────────┘
                                               │
                                               ▼
                                        ┌─────────────┐
                                        │  SQLite /   │
                                        │  FileSystem │
                                        └─────────────┘
```

---

## Quick Start

### Option 1: Docker (권장)

```bash
# 클론
git clone https://github.com/your-repo/yirang-mq.git
cd yirang-mq/docker

# 서비스 시작
./docker-compose.sh up

# 테스트
./docker-compose.sh health
./docker-compose.sh publish telemetry '{"sensor":"A1","temp":25.5}'
./docker-compose.sh consume telemetry
```

### Option 2: 로컬 빌드

```bash
# 요구사항: CMake 3.21+, C++23 컴파일러, vcpkg

# 빌드
./build.sh

# 터미널 1: 데몬 실행
./build/out/MainMQ

# 터미널 2: CLI 테스트
./build/out/yirangmq-cli health
./build/out/yirangmq-cli publish --queue telemetry --message '{"temp":25.5}'
./build/out/yirangmq-cli consume --queue telemetry --consumer-id worker-01
```

---

## 핵심 기능

### 1. 파일 기반 IPC (Mailbox)

네트워크 없이 프로세스 간 통신:

```
ipc/
├── requests/           # 클라이언트 요청
└── responses/          # 서버 응답
    └── <clientId>/
```

- **Atomic Write**: temp → fsync → rename 패턴으로 데이터 무결성 보장
- **Zero Network**: TCP/IP 스택 없이 동작
- **현장 디버깅**: `ls`, `cat`으로 메시지 확인 가능

### 2. 3가지 스토리지 백엔드

| 백엔드 | 특징 | 적합한 환경 |
|--------|------|------------|
| **SQLite** | 트랜잭션, 빠른 조회 | 안정적인 lease/retry 필요 시 |
| **FileSystem** | 파일로 직접 확인 | 디버깅 중심, 의존성 최소화 |
| **Hybrid** | SQLite 인덱스 + 파일 payload | 대용량 메시지 + 빠른 조회 |

### 3. 신뢰성 보장

```
┌─────────┐   lease_next()   ┌─────────┐   timeout    ┌─────────┐
│  ready  │ ───────────────> │ inflight│ ──────────> │  ready  │
└─────────┘                  └─────────┘  (자동복구)   └─────────┘
                                  │
                    ┌─────────────┼─────────────┐
                    │ ack()       │ nack()      │
                    ▼             ▼             ▼
               ┌─────────┐   ┌─────────┐   ┌─────────┐
               │ deleted │   │ delayed │   │   DLQ   │
               └─────────┘   └─────────┘   └─────────┘
```

- **Visibility Timeout**: Consumer 장애 시 메시지 자동 복구
- **Retry with Backoff**: exponential/linear/fixed 백오프
- **Dead Letter Queue**: 재시도 한도 초과 메시지 격리

---

## 사용법

### Docker 환경

```bash
cd docker

# 서비스 관리
./docker-compose.sh up          # 시작
./docker-compose.sh down        # 중지
./docker-compose.sh logs -f     # 로그 확인
./docker-compose.sh status      # 상태 확인

# 메시지 조작
./docker-compose.sh health
./docker-compose.sh publish <queue> '<json>'
./docker-compose.sh consume <queue>

# 컨테이너 내부 CLI
docker exec yirangmq /app/yirangmq-cli status --queue telemetry
docker exec yirangmq /app/yirangmq-cli ack --lease-id <id> --message-key <key>
```

### 로컬 환경

**터미널 1: MainMQ 데몬**
```bash
./build/out/MainMQ
```

**터미널 2: yirangmq-cli**
```bash
# 헬스 체크
./build/out/yirangmq-cli health

# 메시지 발행
./build/out/yirangmq-cli publish \
  --queue telemetry \
  --message '{"sensor":"A1","temp":25.5}' \
  --priority 5

# 메시지 소비 (lease 획득)
./build/out/yirangmq-cli consume \
  --queue telemetry \
  --consumer-id worker-01

# 처리 성공 → ACK
./build/out/yirangmq-cli ack \
  --lease-id <leaseId> \
  --message-key <messageKey>

# 처리 실패 → NACK (재시도)
./build/out/yirangmq-cli nack \
  --lease-id <leaseId> \
  --message-key <messageKey> \
  --reason "parse error" \
  --requeue

# 큐 상태 확인
./build/out/yirangmq-cli status --queue telemetry

# DLQ 조회
./build/out/yirangmq-cli list-dlq --queue telemetry

# DLQ 메시지 재처리
./build/out/yirangmq-cli reprocess --message-key <key>

# 메트릭
./build/out/yirangmq-cli metrics
```

### CLI 옵션

| 옵션 | 설명 | 기본값 |
|------|------|--------|
| `--ipc-root` | IPC 폴더 경로 | `./ipc` |
| `--timeout` | 응답 대기 시간 (ms) | `30000` |
| `--priority` | 메시지 우선순위 | `0` |

---

## 설정

### main_mq_configuration.json

```json
{
  "schemaVersion": "0.1",
  "nodeId": "device-01",
  "backend": "sqlite",

  "paths": {
    "dataRoot": "./data",
    "logRoot": "./logs"
  },

  "ipc": {
    "root": "./ipc",
    "pollIntervalMs": 100,
    "staleTimeoutMs": 30000
  },

  "sqlite": {
    "dbPath": "./data/yirangmq.db",
    "journalMode": "WAL",
    "busyTimeoutMs": 5000
  },

  "lease": {
    "visibilityTimeoutSec": 30,
    "sweepIntervalMs": 1000
  },

  "policyDefaults": {
    "visibilityTimeoutSec": 30,
    "retry": {
      "limit": 5,
      "backoff": "exponential",
      "initialDelaySec": 1,
      "maxDelaySec": 60
    },
    "dlq": {
      "enabled": true,
      "retentionDays": 14
    }
  }
}
```

### 백엔드 선택

```json
// SQLite (권장)
"backend": "sqlite"

// FileSystem
"backend": "filesystem"

// Hybrid (SQLite 인덱스 + 파일 payload)
"backend": "hybrid"
```

---

## 아키텍처

### 컴포넌트 구조

```
┌────────────────────────────────────────────────────────────┐
│                        MainMQ Daemon                        │
├──────────────┬──────────────┬──────────────┬───────────────┤
│MailboxHandler│ QueueManager │Configurations│MessageValidator│
│  (IPC 처리)  │(Lease/Retry) │  (설정 로드)  │   (스키마)     │
└──────┬───────┴──────┬───────┴──────────────┴───────────────┘
       │              │
       └──────────────┘
                │
                ▼
      ┌─────────────────┐
      │  BackendAdapter │
      │ (추상 인터페이스) │
      └────────┬────────┘
               │
  ┌────────────┼────────────┬────────────┐
  ▼            ▼            ▼            ▼
┌───────┐  ┌───────┐  ┌───────┐  ┌───────────┐
│SQLite │  │  FS   │  │Hybrid │  │  (확장)   │
│Adapter│  │Adapter│  │Adapter│  │           │
└───────┘  └───────┘  └───────┘  └───────────┘
```

### 메시지 흐름

```
Producer                 MainMQ                    Backend
   │                        │                         │
   │ 1. Request JSON 작성    │                         │
   │ ──────────────────────>│                         │
   │    ipc/requests/       │                         │
   │                        │ 2. enqueue()            │
   │                        │ ───────────────────────>│
   │                        │    state = "ready"      │
   │ 3. Response JSON       │                         │
   │ <──────────────────────│                         │
   │    ipc/responses/      │                         │


Consumer                 MainMQ                    Backend
   │                        │                         │
   │ 1. consume_next 요청    │                         │
   │ ──────────────────────>│                         │
   │                        │ 2. lease_next()         │
   │                        │ ───────────────────────>│
   │                        │    state = "inflight"   │
   │                        │    lease_until = now+30s│
   │ 3. message + lease     │                         │
   │ <──────────────────────│                         │
   │                        │                         │
   │ 4. ack/nack            │                         │
   │ ──────────────────────>│ 5. ack() / nack()      │
   │                        │ ───────────────────────>│
```

---

## 빌드

### 요구사항

- CMake 3.21+
- C++23 지원 컴파일러 (GCC 13+, Clang 16+)
- vcpkg

### 의존성

```json
{
  "dependencies": [
    "lz4",
    "efsw",
    "sqlite3",
    "nlohmann-json"
  ]
}
```

### 빌드 명령

```bash
# Release 빌드 (기본)
./build.sh

# Debug 빌드
./build.sh -d

# Clean 빌드
./build.sh -c

# 병렬 빌드 (8 jobs)
./build.sh -j 8

# vcpkg 경로 지정
./build.sh -v ~/vcpkg
```

### 출력물

```
build/
├── out/
│   ├── MainMQ              # 데몬 서비스
│   ├── yirangmq-cli        # CLI 클라이언트
│   └── *.json              # 설정 파일
└── lib/
    ├── libBackendAdapter.a
    ├── libUtilities.a
    └── ...
```

---

## 테스트

### Docker 테스트

```bash
cd docker
./docker-compose.sh up
./docker-compose.sh health
./docker-compose.sh publish telemetry '{"test":true}'
./docker-compose.sh consume telemetry
./docker-compose.sh down
```

### 로컬 통합 테스트

```bash
# Mailbox IPC 테스트
./test_mailbox.sh

# 전체 백엔드 모드 테스트
./test_all_modes.sh

# Fault tolerance 테스트
./test_fault.sh
```

---

## 적합한 사용 사례

### 추천

- 레거시 임베디드에서 프로세스 간 메시징이 필요한 경우
- 네트워크 스택 없이 IPC가 필요한 경우
- RabbitMQ/Kafka 등이 과한 저사양 환경
- 전원 차단/재시작이 빈번한 산업용 장비
- 현장에서 파일로 직접 메시지를 확인해야 하는 경우

### 비추천

- 초저지연/하드 리얼타임이 필수인 경우
- 대용량 스트리밍이 필요한 경우
- 멀티 디바이스 분산 메시징 (현재 범위 외)

---

## 프로젝트 구조

```
yirang-mq/
├── MainMQ/                    # 데몬 서비스
│   ├── BackendAdapter/        # 스토리지 백엔드
│   │   ├── BackendAdapter.h   # 추상 인터페이스
│   │   ├── SQLiteAdapter.*    # SQLite 구현
│   │   ├── FileSystemAdapter.*# FileSystem 구현
│   │   └── HybridAdapter.*    # Hybrid 구현
│   ├── MailboxHandler.*       # IPC 처리
│   ├── QueueManager.*         # Lease/Retry 관리
│   ├── Configurations.*       # 설정 로더
│   └── main.cpp               # 진입점
├── Samples/
│   └── yirangmq-cli/          # CLI 클라이언트
├── CommonLibrary/             # 공용 라이브러리
│   ├── Utilities/             # Logger, File, JSON 등
│   ├── DataBase/              # SQLite 래퍼
│   └── ThreadPool/            # 스레드풀
├── docker/                    # Docker 설정
│   ├── Dockerfile
│   ├── docker-compose.yml
│   └── docker-compose.sh
└── docs/                      # 문서
    ├── OPERATIONS.md          # 운영 가이드
    └── TROUBLESHOOTING.md     # 장애 대응
```

---

## 문서

- [운영 가이드](docs/OPERATIONS.md) - 디렉토리 구조, 백업, 모니터링
- [장애 대응](docs/TROUBLESHOOTING.md) - 문제 해결 시나리오
- [코드 컨벤션](CODE_CONVENTIONS.md) - 개발 가이드라인

---

## Contributing

Issues와 PR을 환영합니다.
특히 임베디드/레거시 환경에서의 실제 사용 경험에 기반한 피드백을 환영합니다.

---

## License

MIT License

---

## Name

"Yi-Rang(이랑)"은 프로젝트 이름이자, 딸의 이름에서 유래했습니다.
