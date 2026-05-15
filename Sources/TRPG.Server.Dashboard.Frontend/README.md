# TRPG.Server Dashboard Frontend

React + Vite 기반 Dashboard 프론트엔드 프로젝트입니다.

## 실행

```powershell
npm install
npm run dev
```

## 빌드

```powershell
npm run build
```

## 역할

- 화면과 사용자 상호작용을 담당합니다.
- Backend HTTP API를 호출합니다.
- 서버 상태 실시간 조회는 이후 `/api/server/status` 계약이 확정되면 polling으로 연결합니다.
