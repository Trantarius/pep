
# initial simple design and example, may change

# line comments start with #

##
block comments with double #
##

# file include
import path/to/file;

# types/global declaration

int32 myInt;
int8 myByte=0;
uint64 unsignedInt;
int defaultInt; #make this 64 bits

float32 myFloat=2;
float64 myDouble=1.2;
float defaultFloat;#make this 64 bits

string myString = "this is a string";

#verbose function declaration

(float,float->float) addFloats=(float a,float b->float){
    return a+b;
}

#easy function declaration

func mulInts = (int a, int b -> int){
    return a*b;
}

#variadic function
func variadicFunc = (int...nums -> int){
    int sum=0;
    #foreach loop
    for int x in nums{
        sum+=x;
    }

    int prod=1;
    #for range loop
    for int n in 0...nums.size {
        prod *= nums[n];
    }

    return sum/prod;
}

#entrypoint
func main = (string...args->void){
    if(args.size<1){
        print("need an arg");
        #exit program with code 1 (built-in in namespace pep)
        pep:exit(1);
    }

    #function call, convert string to int
    fibonacci(string:parseInt(args[0]));

}

func fibonacci=(int count->void){
    int a=0;
    int b=1;

    for( int n in 0...count){
        int c=a+b;
        a=b;
        b=c;
        print(c);
    }
}
