// 1. 괄호는 항상 아래로 내려 작성
void parentheses_convention()
{
    return;
}

// 1-1. 괄호가 비어있다면 옆으로 작성
void empty_parentheses_convention() {}

// 2. 초기화와 동시에 대상을 사용한다면 아래 괄호를 열어서 작성
init_class* data_init_convention = new init_class();
{
    data_init_convention->init();
}

// 3. 변수 네이밍은 스네일 케이스 소문자로 작성
int value_convention = 0;

// 4. enum의 값은 스네일 케이스 대문자로 작성
enum enum_convention
{
    ENUM_VALUE_CONVENTION
}

// 4. 람다식은 항상 아래로 내려 작성
lambda_callback_func_convention([this](lambda_convention lc)
{
    std::cout << "lambda_convention" << std::endl;
});

// 5. 주석은 오직 // <- 이것만 사용
// comment_convention

// 6. 클래스 구조는 아래와 같이 작성
// 6-1. 정의, 생성자/소멸자, 함수, 변수 순서로
// 6-2. 각 카테고리별로 public, protected, private 접근자 개별 작성 (내용이 없다면 접근자 작성 생략)
class class_convention
{
public:

    enum inner_enum {}
    class inner_class {}

protected:

    //

private:

    //

public:

    class_convention() = default;
    ~class_convention() = default;

protected:

    //

private:

    //

public:

    void class_convention_func();

protected:

    //

private:

    //

public:

    int class_convention_value;

protected:

    //

private:

    //
}

// 7. 한글 1개 당 2타, 영어+기호 1개 당 1타로 계산해서 글자수가 90자를 넘어가면 아래로 내려서 작성.
void long_sentense_func_conventionnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn
(
    int param1 a,
    int param2 b,
    int param3 c
)

// 8. 헤더는 현재 프로젝트의 파일, 다른 프로젝트의 파일, 외부 라이브러리의 파일 순서로 선언
#include "curr_project_file.h"
#include "other_project/file.h"
#include <external/file.h>