
#include "CommonDefs.h"
#include "Feed.h"
#include "Book.h"
#include "InputReader.h"
#include "Logger.h"
#include "MarketDataConsumer.h"

using namespace std;

Logger LOG("/tmp/MarketDataMergerLog");

class MainApp
{
public:
	MainApp(vector<string> inputFiles, int processingGroupCount) : _reporter(ReporterPtr(new KnowsAboutFeedsStandardOutputReporter(inputFiles.size()))),
													_consumer(new MarketDataConsumer(processingGroupCount, _reporter))
	{

		FeedID feedid = 0;
		for(const string& file : inputFiles)
		{
			FeedPtr feed{new Feed(InputReaderPtr(new FileInputReader(file)), feedid)};
			_feed.addFeed(std::move(feed));
			feedid++;
		}
		_feed.registerNewRecordCB(std::bind(&MarketDataConsumer::push, _consumer, placeholders::_1));
	}
	~MainApp() {}
	void start()
	{
		_consumer->start();
		_feed.start();
		//loop until done
		_feed.join();
		_consumer->join();
		_reporter->requestStop();
		_reporter->join();

		// report different statistics
		_reportBookStatistics();
	}

private:
	void _reportBookStatistics()
	{
		cout << "\n+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
		cout << "Reporting book statistics:\n";
		cout << "Latency(microsec) on market update changing the top of the book.\n";
		cout << "Latency(microsec) is measured from first reading the market data entry from one of the feeds until the point we updated the book.\n";
		cout << "\n";
		unordered_map<string, BookStatistics> bookstats = _consumer->getBookStatistics();
		for(auto& p : bookstats)
		{
			p.second.sortLatencies();
			cout << p.second.toString() << endl;
		}
		cout << "\nEnd of Book Statistics\n";
		cout << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
	}

private:
	ReporterPtr								_reporter;
	FeedManager 							_feed;
	MarketDataConsumerPtr 					_consumer;
};



int main(int argc, char** argv)
{
	auto start = chrono::system_clock::now();
	vector<string> infiles;
	for(int i=1;i<argc;i++)
	{
		infiles.push_back(argv[i]);
	}

	MainApp app(infiles, 6);
	app.start();

	return 0;
}
