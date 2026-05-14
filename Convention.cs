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

// 4. enum의 값은 파스칼 케이스로 작성
// enum 클래스의 이름은 접미사로 'Type'을 붙여 작성
enum EnumConventionType
{
    EnumValueConvention
}

// 4. 람다식은 항상 아래로 내려 작성
LambdaCallbackFuncConvention([this](LambdaConvention lc)
{
    std::cout << "LambdaConvention" << std::endl;
});

// 5. 주석
public class ClassConvention
{
    /// <summary>
    /// 클래스 맴버의 경우 이런식으로 
    /// </summary>
    private int conventionValue1;

    /// <summary>
    /// 클래스 맴버의 경우 이런식으로 
    /// </summary>
    public void ClassConventionFunc()
    {
        // 함수 내부의 경우 이런식으로
        std::cout << "ClassConventionFunc" << std::endl;
    }
}

// 6. 클래스 구조는 아래와 같이 작성 + 순서 포함, 각 항목마다 한 줄 더 띄어쓰기
public class ClassConvention
{
    // 1. 클래스에 포함되는 각종 struct, enum, class...
    public enum InnerEnum {}


    // 2. 맴버 변수는 카멜 케이스
    private int conventionValue1;

    private int conventionValue2;


    // 3. 속성은 파스칼 케이스
    public int ConventionProperty;


    // 4. 생성자
    public class InnerClass {}


    // 5. 함수는 파스칼 켕스
    public void ClassConventionFunc();
}

// 7. 한글 한글자에 2타, 영어+기호 한글자에 1타로 계산했을 때, 한줄에 150타 초과 시 초과된 부분부터 내려쓰기
void LongSentenseFuncConventionNnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn
(
    int param1A,
    int param2B,
    int param3C
)

// 8. 가벼운 얼리 리턴은 if문 바로 옆에 짧게 return을 작성
if (isChecked) return true;


// 9. null체크는 로직에서의 문제가 아니라면 하지마
if (component) <- 단순히 초기화 실패 오류