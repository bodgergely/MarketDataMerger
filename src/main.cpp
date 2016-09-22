
#include "CommonDefs.h"
#include "Feed.h"
#include "Book.h"
#include "InputReader.h"
#include "Logger.h"
#include "MarketDataConsumer.h"

using namespace std;


class MainApp
{
public:
	MainApp(vector<string> inputFiles, int processingGroupCount) : _consumer(new MarketDataConsumer)

	{

		FeedID feedid = 0;
		for(const string& file : inputFiles)
		{
			FeedPtr feed{new Feed(new FileInputReader(file), feedid)};
			_feed.addFeed(std::move(feed));
			feedid++;
		}
		_feed.registerNewRecordCB(std::bind(&MarketDataConsumer::read, _consumer, placeholders::_1));
	}
	~MainApp() {}
	void start()
	{
		_feed.start();
		// report different statistics
	}

	void reportBookStatistics()
	{
		cout << "\n+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
		cout << "Reporting book statistics:\n";
		_consumer->writeBookStatistics();
		cout << "\nEnd of Book Statistics\n";
		cout << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
	}


private:
	FeedManager 							_feed;
	MarketDataConsumerPtr 					_consumer;
};



int main(int argc, char** argv)
{
	cout << "Record: " << sizeof(Record) << endl;
	cout << "TimePoint: " << sizeof(TimePoint) << endl;
	auto start = chrono::system_clock::now();
	vector<string> infiles;
	for(int i=1;i<argc;i++)
	{
		infiles.push_back(argv[i]);
	}

	MainApp app(infiles, 6);
	app.start();
	auto feedProcessedTimePoint = chrono::system_clock::now();
	app.reportBookStatistics();
	auto statsReportedTimePoint = chrono::system_clock::now();

	cout << "Timings:\n";
	cout << "Until last record update: " << chrono::duration_cast<chrono::milliseconds>(feedProcessedTimePoint - start).count() <<
			" (millisecs), Book stats write to cout: " << chrono::duration_cast<chrono::microseconds>(statsReportedTimePoint - feedProcessedTimePoint).count() <<
			" (microsecs), Total time: " << chrono::duration_cast<chrono::milliseconds>(statsReportedTimePoint - start).count() << " (millisecs)\n";

	return 0;
}
