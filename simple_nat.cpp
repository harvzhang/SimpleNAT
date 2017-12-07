/*
 * simple_nat.cpp
 * Author: Harvey Zhang
 *
 * Simple NAT storage and translation code
 */

#include <iostream>
#include <unordered_map>
#include <string>
#include <vector>
#include <fstream>

using namespace std;

//output status for save function
enum Status {INVALID, OK, NO_MATCH};

/*
 * Class for the NAT key value pair storage
 */
class NATStore {
	unordered_map<string,string> dictionary;

public:
	Status save (string entry);
	void save (string key, string value);
	Status translate (string key, string& value);

private:
	bool is_valid(string pair, bool is_value);
	bool is_ip(string ip);
	bool is_port(string port);
	bool is_pint(string input);
	vector<string> parse (string entry, string delimiter);
	vector<string> parse_entry(string entry);
};

/*
 * Parses a given entry by a delimiter, code inspired by:
 * https://stackoverflow.com/questions/14265581/parse-split-a-
 * string-in-c-using-string-delimiter-standard-c
 */
vector<string> NATStore::parse(string entry, string delimiter) {
	size_t i;
	vector<string> parsed;
	while ((i = entry.find(delimiter)) != string::npos) {
    	string token = entry.substr(0, i);
    	parsed.push_back(token);
    	entry.erase(0, i + delimiter.length());
    }

    parsed.push_back(entry);

    return parsed;
} 

/*
 * Checks to see if the input string represents a positive integer
 * Code inspired by: https://stackoverflow.com/questions/2844817/how-do-i-check-if-a-c-string-is-an-int
 */
bool NATStore::is_pint(string input) {
	return input.find_first_not_of( "0123456789" ) == string::npos;
}

/*
 * Checks to see if the string is a valid port
 */
bool NATStore::is_port(string port) {
	if (port == "*") return true;
	if (!is_pint(port)) return false;

	int p = stoi(port);
	if (p > 65535 || p < 0) return false;

	return true;
}

/*
 * Checks to see if the string is a valid ip
 */
bool NATStore::is_ip(string ip) {
	if (ip == "*") return true;
	vector<string> parsed = parse(ip, ".");

	if (parsed.size() != 4) return false;

	for (int i = 0; i < parsed.size(); i++) {
		if (!is_pint(parsed[i])) return false;
		int p = stoi(parsed[i]);
		if (p > 255 || p < 0) return false;
	}

	return true;
}

/*
 * Checks to see if the ip/port pair is valid
 * is_value = true if the ip.port pair is the "value" in
 * the NAT entry
 */
bool NATStore::is_valid (string pair, bool is_value) {
	vector<string> parsed = parse(pair, ":");

	if (parsed.size() != 2) 
		return false;
	if (is_value && (parsed[0] == "*" || parsed[1] == "*"))
			return false;
	if (parsed[0] == "*" && parsed[1] == "*")
		return false;
	if (!is_ip(parsed[0]) || !is_port(parsed[1]))
		return false;
	
	return true;
}

/*
 * Parses the ip/port pairs and checks format
 */
vector<string> NATStore::parse_entry(string entry) {

	vector<string> parsed = parse(entry, ",");

    if (parsed.size() != 2) return vector<string>();

    if (!is_valid(parsed[0], false) || 
    	!is_valid(parsed[1], true)) 
    	return vector<string>();

    return parsed;
}

/*
 * Checks if the formats are correct before saving
 */
Status NATStore::save(string entry) {
	vector<string> parsed = parse_entry(entry);

	if (parsed.empty()) return INVALID;

	save (parsed[0], parsed[1]);

	return OK;
}

/*
 * Saves the key value pair into the hash table,
 * overwrites key with new value
 */
void NATStore::save(string key, string value) {
	dictionary[key] = value;
}

/*
 * Checks for correct key format and retrieves from
 * the hash table if the key exists
 *
 * returns string "not_found" if key does not exist
 */
Status NATStore::translate(string key, string& value) {
	vector<string> parsed = parse(key, ":");

	if (!is_ip(parsed[0]) || !is_port(parsed[1]))
		return INVALID;

	if (parsed[0] == "*" || parsed[1] == "*")
		return INVALID;

	if(dictionary.find(key) != dictionary.end()) {
		value = dictionary[key];
		return OK;
	}

	string key_port = parsed[0] + ":*";
 	if (dictionary.find(key_port) != dictionary.end()) {
 		value = dictionary[key_port];
 		return OK;
 	}
		

	string key_ip = "*:" + parsed[1];
	if (dictionary.find(key_ip) != dictionary.end()){
		value = dictionary[key_ip];
		return OK;
	}

	return NO_MATCH;
}


/*
 * Testing of the Simple NAT Code
 */

void test() {

	NATStore nat = NATStore();

	//Case1: input only have one ip:port pair
	if (nat.save("192.168.0.1:80") !=  INVALID){
		cout << "Case 1: Fail" <<endl;
	} else {
		cout << "Case 1: Pass" <<endl;
	}

	//Case2: input have non-numeric port
	if (nat.save("10.0.1.1:port,192.168.0.1:80") !=  INVALID){
		cout << "Case 2: Fail" <<endl;
	} else {
		cout << "Case 2: Pass" <<endl;
	}

	//Case3: input have shorter ip
	if (nat.save("1.0.1:8082,192.168.0.3:80") != INVALID){
		cout << "Case 3: Fail" <<endl;
	} else {
		cout << "Case 3: Pass" <<endl;
	}

	//Case4: input have out of range ip / port
	if (nat.save("300.0.1.1:8082,192.168.0.3:80") != INVALID){
		cout << "Case 4: Fail" <<endl;
	} else {
		cout << "Case 4: Pass" <<endl;
	}

	//Case5: input have three ip:port
	if (nat.save("10.0.1.1:8082,192.168.0.3:80,192.168.0.3:85") != INVALID){
		cout << "Case 5: Fail" <<endl;
	} else {
		cout << "Case 5: Pass" <<endl;
	}

	//Case6: input is of form *:*
	if (nat.save("*:*,192.168.0.1:80") != INVALID){
		cout << "Case 6: Fail" <<endl;
	} else {
		cout << "Case 6: Pass" <<endl;
	}

	//Case7: input value contains *
	if (nat.save("10.0.1.1:8080,*:80") != INVALID){
		cout << "Case 7: Fail" <<endl;
	} else {
		cout << "Case 7: Pass" <<endl;
	}


	string response;

	//Case8: translate not found
	if (nat.save("10.0.1.1:8080,192.168.0.1:80") !=  OK){
		cout << "Case 8: Fail" <<endl;
	}

	if (nat.translate("10.0.1.1:8085", response) == NO_MATCH)
		cout << "Case 8:Pass" <<endl;
	else 
		cout << "Case 8:Fail" <<endl;

	//Case9: translate input have *
	if (nat.save("*:8082,192.168.0.1:81") !=  OK){
		cout << "Case 9: Fail" <<endl;
	}

	;
	if (nat.translate("*:8082", response) == INVALID)
		cout << "Case 9:Pass" <<endl;
	else 
		cout << "Case 9:Fail" <<endl;

	//Case10: translate input with * correctly (ip/port)
	if (nat.translate("10.0.1.1:8082",response) != OK)
		cout << "Case 10:Fail" <<endl;
	if (response == "192.168.0.1:81") 
		cout << "Case 10:Pass" <<endl;
	else 
		cout << "Case 10:Fail" <<endl;


	//Case11: translate normal case correctly
	if (nat.translate("10.0.1.1:8080", response) != OK){
		cout << "Case 11:Fail" <<endl;
	}
	if (response == "192.168.0.1:80") 
		cout << "Case 11:Pass" <<endl;
	else 
		cout << "Case 11:Fail" <<endl;

	if (nat.save("10.0.1.2:*,192.168.0.1:83") !=  OK){
		cout << "Case 12: Fail" <<endl;
	}

	if(nat.translate("10.0.1.2:8085", response) != OK) {
		cout << "Case 12:Fail" <<endl;
	}
	if (response == "192.168.0.1:83") 
		cout << "Case 12:Pass" <<endl;
	else 
		cout << "Case 12:Fail" <<endl;
}

int main() {

	//Runs tests to determine the correctness of code
	//test();

	NATStore nat = NATStore();

	ifstream natfile("NAT");

	string line;
	while (getline(natfile, line)) {
		if (line != "") {
			if (nat.save(line)!= OK)
				cout <<"Error: " + line + 
					" is not valid input" <<endl;
		}
	}

	ifstream flowfile("FLOW");
	ofstream outfile;
	outfile.open("OUTPUT");

	string value;
	while (getline(flowfile, line)) {
		if (line != ""){
			string value;
			Status stat = nat.translate(line, value);
			if (stat == INVALID)
				outfile << "query " + line + " format is incorrect\n";
			else if (stat == NO_MATCH)
				outfile << "No nat match for " + line + "\n";
			else
				outfile << line + " -> " + value + "\n";
		}
	}

	outfile.close();
}


