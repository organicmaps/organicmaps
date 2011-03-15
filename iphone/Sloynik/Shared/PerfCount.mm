#import "PerfCount.h"
#import <map>
#import <string>
#import <utility>
using namespace std;

void LogTimeCounter(const char * counter, const char * message)
{
	static map<string, double> counters;
	const double t = [NSDate timeIntervalSinceReferenceDate];
	const string s = counter;
	map<string, double>::iterator it = counters.find(s);
	if (it == counters.end())
	{
		NSLog(@"LogTimeCounter: %s %s start_point", counter, message);
		counters.insert(make_pair(s, t));
	}
	else
	{
		NSLog(@"LogTimeCounter: %s %s %f", counter, message, t - it->second);
		it->second = t;		
	}
}
