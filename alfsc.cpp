/*
ALFSC - Alignment-free Sequence Comparison
Version 0.0.1
Written by Dr. Martin Vickers (martin.vickers@jic.ac.uk)

MIT License

Copyright (c) 2017 Martin James Vickers

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "common.h"
#include "seq.cpp"
#include "utils.cpp"

/*
Parse our commandline options
*/
seqan::ArgumentParser::ParseResult parseCommandLine(ModifyStringOptions & options, int argc, char const ** argv)
{
	seqan::ArgumentParser parser("alfsc");
	addOption(parser, seqan::ArgParseOption("k", "klen", "Kmer Length.", seqan::ArgParseArgument::INTEGER, "INT"));
	setDefaultValue(parser, "klen", "3");
	addOption(parser, seqan::ArgParseOption("d", "debug", "Debug Messages."));
	addOption(parser, seqan::ArgParseOption("q", "query-file", "Path to the file containing your query sequence data.\n", seqan::ArgParseArgument::INPUT_FILE, "IN"));
	setValidValues(parser, "query-file", toCString(concat(getFileExtensions(SeqFileIn()), ' ')));
	addOption(parser, seqan::ArgParseOption("r", "reference-file", "Path to the file containing your reference sequence data.", seqan::ArgParseArgument::INPUT_FILE, "IN"));
	setValidValues(parser, "reference-file", toCString(concat(getFileExtensions(SeqFileIn()), ' ')));
	addOption(parser, seqan::ArgParseOption("p", "pairwise-file", "Path to the file containing your sequence data which you will perform pairwise comparison on.", seqan::ArgParseArgument::INPUT_FILE, "IN"));
	setValidValues(parser, "pairwise-file", toCString(concat(getFileExtensions(SeqFileIn()), ' ')));
	addOption(parser, seqan::ArgParseOption("m", "markov-order", "Markov Order", seqan::ArgParseArgument::INTEGER, "INT"));
	addOption(parser, seqan::ArgParseOption("o", "output-file", "Output file.", seqan::ArgParseArgument::OUTPUT_FILE, "OUT"));
	setRequired(parser, "output-file");
	setDefaultValue(parser, "markov-order", "1");
	addOption(parser, seqan::ArgParseOption("n", "num-hits", "Number of top hits to return", seqan::ArgParseArgument::INTEGER, "INT"));
	setDefaultValue(parser, "num-hits", "10");
	addOption(parser, seqan::ArgParseOption("t", "distance-type", "The method of calculating the distance between two sequences.", seqan::ArgParseArgument::STRING, "STR"));
	setValidValues(parser, "distance-type", "d2 kmer d2s d2star manhattan chebyshev hao dai");
	setDefaultValue(parser, "distance-type", "d2");
	addOption(parser, seqan::ArgParseOption("f", "output-format", ".", seqan::ArgParseArgument::STRING, "STR"));
	setValidValues(parser, "output-format", "tabular");
        setDefaultValue(parser, "output-format", "tabular");
	addOption(parser, seqan::ArgParseOption("nr", "no-reverse", "Do not use reverse compliment."));
	addOption(parser, seqan::ArgParseOption("c", "num-cores", "Number of Cores.", seqan::ArgParseArgument::INTEGER, "INT"));
	addOption(parser, seqan::ArgParseOption("u", "use-ram", "Use RAM to store reference counts once computed. Very fast but will use a lot of RAM if you have a large reference and/or large kmer size."));
	setDefaultValue(parser, "num-cores", "1");
	setShortDescription(parser, "Alignment-free sequence comparison.");
	setVersion(parser, "0.0.3");
	setDate(parser, "September 2016");
	addUsageLine(parser, "-q query.fasta -r reference.fasta -o results.txt [\\fIOPTIONS\\fP] ");
	addUsageLine(parser, "-p mydata.fasta -o results.txt [\\fIOPTIONS\\fP] ");
	addDescription(parser, "Perform Alignment-free k-tuple frequency comparisons from two fasta files.");

	seqan::ArgumentParser::ParseResult res = seqan::parse(parser, argc, argv);

	// Only extract options if the program will continue after parseCommandLine()
	if (res != seqan::ArgumentParser::PARSE_OK)
		return res;

	//begin extracting options
        getOptionValue(options.klen, parser, "klen");
        getOptionValue(options.nohits, parser, "num-hits");
        getOptionValue(options.markovOrder, parser, "markov-order");
        getOptionValue(options.type, parser, "distance-type");
        options.noreverse = isSet(parser, "no-reverse");
	options.debug = isSet(parser, "debug");
	options.useram = isSet(parser, "use-ram");
	getOptionValue(options.queryFileName, parser, "query-file");
	getOptionValue(options.referenceFileName, parser, "reference-file");
	getOptionValue(options.pairwiseFileName, parser, "pairwise-file");
	getOptionValue(options.outputFileName, parser, "output-file");
	getOptionValue(options.num_threads, parser, "num-cores");
	getOptionValue(options.output_format, parser, "output-format");

	if(isSet(parser, "pairwise-file")){
		if(isSet(parser, "reference-file") == true || isSet(parser, "query-file") == true)
		{
			cerr << "If you are performing a pairwise comparison, you do not need to specify a query (-q) and a reference (-r) file. If you are performing a reference/query based search you do not need to specify a pairwise-file (-p). See alfsc -h for details." << endl;
			return seqan::ArgumentParser::PARSE_ERROR;
		}
	}
	if(isSet(parser, "reference-file") == true && isSet(parser, "query-file") == false)
	{
		cerr << "You have specified a reference (-r) file but not a query (-q) file. See alfsc -h for details." << endl;
		printHelp(parser);
		return seqan::ArgumentParser::PARSE_ERROR;
	}
	if(isSet(parser, "reference-file") == false && isSet(parser, "query-file") == true)
        {
                cerr << "You have specified a query (-q) file but not a reference (-r) file. See alfsc -h for details." << endl;
		printHelp(parser);
                return seqan::ArgumentParser::PARSE_ERROR;
        }
	if(isSet(parser, "reference-file") == false && isSet(parser, "query-file") == false && isSet(parser, "pairwise-file") == false)
	{
		cerr << "You have not specifed any input file. See alfsc -h for details." << endl;
		printHelp(parser);
                return seqan::ArgumentParser::PARSE_ERROR;
	}
	return seqan::ArgumentParser::PARSE_OK;
}

//this is where we do stuff
void mainloop(ModifyStringOptions options)
{
	IupacString queryseq;
	CharString queryid;
	SeqFileIn queryFileIn;
	open(queryFileIn, (toCString(options.queryFileName)));

        CharString refid;
        IupacString refseq;
        SeqFileIn refFileIn;
	open(refFileIn, (toCString(options.referenceFileName)));

	while(!atEnd(queryFileIn))
	{
		readRecord(queryid, queryseq, queryFileIn);
		Seq qryseqobj(queryseq, queryid, options.noreverse);

		while(!atEnd(refFileIn))
		{
			readRecord(refid, refseq, refFileIn);
			Seq refseqobj(refseq, refid, options.noreverse);
	
		//	qryseqobj.printMarkov(options.klen, options.markovOrder);
		//	refseqobj.printMarkov(options.klen, options.markovOrder);
	
			cout << "KMER: " << euler(refseqobj, qryseqobj, options) << endl;
			cout << "D2: " << d2(refseqobj, qryseqobj, options) << endl;
			cout << "D2S: " << d2s(refseqobj, qryseqobj, options) << endl;

		//	refseqobj.printMarkov(options.klen, options.markovOrder);
		//	qryseqobj.printMarkov(options.klen, options.markovOrder);
		}
	}
}

int main(int argc, char const ** argv)
{
	//parse our options
	ModifyStringOptions options;
	seqan::ArgumentParser::ParseResult res = parseCommandLine(options, argc, argv);
	
	// If parsing was not successful then exit with code 1 if there were errors.
	// Otherwise, exit with code 0 (e.g. help was printed).
	if (res != seqan::ArgumentParser::PARSE_OK)
		return res == seqan::ArgumentParser::PARSE_ERROR;

	//THE PLAN

	//I need an object/class which can store the sequence and do some operations on it
		//if we have the RAM to precompute, then we can store the reference objects, but if not, we just count every time
		//multithread this actual counts and calculations rather than the number of comparisons at any one time. Maybe we can do both but will see.

	mainloop(options);

	return 0;
}
