/*
	��ȣ��
	2016.12.04
	�Ͻ�Ʈ�� ��
*/

const int max = 999;

void main()
{
	int i, j, t, k;

	i = 100;
	while(i <= max){
	   j=i/100;
	   k = (i%100)/10;
	   t = (i%100)%10;
	   if(i == (j*j*j + k*k*k + t*t*t)) write(i);
	   ++i;
	}
}