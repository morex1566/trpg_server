// 1. 괄호는 항상 아래로 내려 작성
void ParenthesesConvention()
{
    return;
}

// 1-1. 괄호가 비어있다면 옆으로 작성
void EmptyParenthesesConvention() {}

// 2. 초기화와 동시에 대상을 사용/변경한다면 아래 괄호를 열어서 작성
InitClass* dataInitConvention = new InitClass();
{
    dataInitConvention->Init();
}

// 3. 변수 네이밍은 소문자 시작 카멜 케이스로 작성
int valueConvention = 0;

// 4. enum의 값은 스네일 케이스 대문자로 작성
enum EnumConvention
{
    ENUM_VALUE_CONVENTION
}

// 4. 람다식은 항상 아래로 내려 작성
LambdaCallbackFuncConvention([this](LambdaConvention lc)
{
    std::cout << "LambdaConvention" << std::endl;
});

// 5. 주석은 오직 // <- 이것만 사용
// CommentConvention

// 6. 클래스 구조는 아래와 같이 작성
// 6-1. 정의, 변수, 함수 순으로 작성
public class ClassConvention
{
    public enum InnerEnum {}
    public class InnerClass {}

    // 맴버 변수는 카멜 케이스
    private int conventionValue;

    // 속성은 파스칼 케이스
    public int ConventionProperty;

    // 함수는 파스칼 켕스
    public void ClassConventionFunc();
}

// 7. 한글 2타, 영어+기호 1타 계산, 90자 초과 시 내려쓰기
void LongSentenseFuncConventionNnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn
(
    int param1A,
    int param2B,
    int param3C
)