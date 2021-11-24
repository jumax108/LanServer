#pragma once

enum class SERVER_ERROR {

	// 서버에 설정된 동시접속자 최대치에 도달하여 새로운 접속 요청을 받을 수 없습니다.	
	SESSION_FULL = 1

};
