#pragma once

enum class SERVER_ERROR {

	// 서버에 설정된 동시접속자 최대치에 도달하여 새로운 접속 요청을 받을 수 없습니다.	
	SESSION_FULL = 1,

	// 요청된 ID와 일치하는 세션이 없습니다.
	NO_SESSION_BY_ID = 2,
};
