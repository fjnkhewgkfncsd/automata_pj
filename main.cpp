#include <iostream>
#include <set>
#include <map>
#include <string>
#include <fstream>
#include <cstdlib>
#include <sstream>
#include <cstdio>  

using namespace std;

class FiniteAutoMaton {
    protected:
        set<string> states;
        set<char> alphabets;
        string startState;
        set<string> acceptingStates;
        string name;
        int numOfStates;
        int numOfAlphabet;
        int numOfAcceptingStates;
        int id;
    public:
        virtual void loadFromDatabase(int id) = 0;
        virtual bool simulate(const string& input) = 0;
        virtual void saveToDatabase() = 0;
        virtual ~FiniteAutoMaton() = default;
        
        virtual void addSymbol(char symbol){
            alphabets.insert(symbol);
        }
        virtual void addAcceptingStates(const string& state){
            acceptingStates.insert(state);
        }
        virtual void displayState() const {
            for(const auto& state : states){
                if(state ==startState){
                    cout << state +"* ";
                }else{
                    cout << state << " ";
                }
            }
        }
        virtual void displaySymbol(){
            for(const char symbol : alphabets){
                cout << symbol << " ";
            }
        }
        virtual void addStates(string& state){
            states.insert(state);
        }
        virtual void setNumOfState(int num){
            numOfStates = num;
        }
        virtual void setNumOfAlphabet(int num){
            numOfAlphabet = num;
        }
        virtual int getNumOfState(){
            return numOfStates;
        }
        virtual int getNumOfAlphabet(){
            return numOfAlphabet;
        }
        virtual set<string>& getStates(){
            return states;
        }
        virtual set<char> getAlphabet(){
            return alphabets;
        } 
        virtual void setStartState(const string& state){
            startState = state ;
        }
        virtual void setAcceptingStates(const string& state){
            acceptingStates.insert(state);
        }
        virtual void setNumOfAcceptingState(int num){
            numOfAcceptingStates = num;
        }
        virtual int getNumOfAcceptingState(){
            return numOfAcceptingStates;
        }
};

class DFA : public FiniteAutoMaton {
    private: 
        map<pair<string,char>, string> transitions;
        
    public:
        string toJSON(const string& name) const {
            stringstream json;
            json << "{\n";
            json << "  \"name\": \"" << name << "\",\n";
            json << "  \"numOfStates\": " << numOfStates << ",\n";
            json << "  \"numOfAlphabet\": " << numOfAlphabet << ",\n";
            json << "  \"numOfAcceptingStates\": " << numOfAcceptingStates << ",\n";
            
            // States array
            json << "  \"states\": [";
            bool first_state = true;
            for (const auto& state : states) {
                if (!first_state) json << ", ";
                json << "\"" << state << "\"";
                first_state = false;
            }
            json << "],\n";
            
            // Start state
            json << "  \"startState\": \"" << startState << "\",\n";
            
            // Alphabet array
            json << "  \"alphabet\": [";
            bool first_symbol = true;
            for (const auto& symbol : alphabets) {
                if (!first_symbol) json << ", ";
                json << "\"" << string(1, symbol) << "\"";
                first_symbol = false;
            }
            json << "],\n";
            json << "  \"acceptingStates\": [";
            bool first_acceptingState = true;
            for (const auto& state : acceptingStates) {
                if (!first_acceptingState) json << ", ";
                json << "\"" << state << "\"";
                first_acceptingState = false;
            }
            json << "],\n";
            json << "  \"transitions\": [";
            bool first_transition = true;
            for (const auto& transition : transitions) {
                if (!first_transition) json << ", ";
                // transition.first is pair<string,char>, transition.second is string
                json << "[\"" << transition.first.first << "\", \"" 
                        << string(1, transition.first.second) << "\", \"" 
                        << transition.second << "\"]";
                first_transition = false;
            }
            json << "]\n";
            json << "}";
            return json.str();
        }
        // Parse JSON and populate DFA
        bool fromJSON(const string& jsonFile) {
            ifstream file(jsonFile);
            if (!file.is_open()) {
                cout << "❌ Failed to open JSON file: " << jsonFile << endl;
                return false;
            }
            string line, content;
            while (getline(file, line)) {
                content += line;
            }
            file.close();
            try {
                // Extract name
                size_t namePos = content.find("\"name\": \"");
                if (namePos != string::npos) {
                    namePos += 9;
                    size_t endPos = content.find("\"", namePos);
                    name = content.substr(namePos, endPos - namePos);
                }
                
                // Extract startState
                size_t startPos = content.find("\"startState\": \"");
                if (startPos != string::npos) {
                    startPos += 15;
                    size_t endPos = content.find("\"", startPos);
                    startState = content.substr(startPos, endPos - startPos);
                }
                
                return true;
            } catch (...) {
                cout << "❌ Error parsing JSON file" << endl;
                return false;
            }
        }
        
        void saveToDatabase() override {
            string dfaName;
            cout << "Enter a name for this DFA: ";
            cin >> dfaName;
            
            // Create JSON file
            string jsonData = toJSON(dfaName);
            string tempFile = "temp_dfa_save.json";
            
            ofstream jsonFile(tempFile);
            if (!jsonFile.is_open()) {
                cout << " Failed to create temporary JSON file!" << endl;
                return;
            }
            jsonFile << jsonData;
            jsonFile.close();
            
            // Execute Python script
            string command = "python db_operation.py insert " + tempFile;
            cout << "💾 Saving DFA to database..." << endl;
            
            // Use simple system() call instead of popen
            int result = system(command.c_str());
            
            // Cleanup temporary file
            remove(tempFile.c_str());
            
            // Just check the result
            if (result == 0) {
                cout << "✅ DFA '" << dfaName << "' saved to database successfully!" << endl;
            } else {
                cout << " Failed to save DFA to database!" << endl;
            }
        }
        
        void loadFromDatabase(int id) override {
            string tempFile = "temp_dfa_load.json";
            string command = "python db_operation.py load " + to_string(id) + " " + tempFile;
            
            cout << "📂 Loading DFA from database (ID: " << id << ")..." << endl;
            
            // ✅ FIXED: Use system() instead of popen
            int result = system(command.c_str());
            
            if (result == 0) {
                // Check if file was created successfully
                ifstream checkFile(tempFile);
                if (checkFile.good()) {
                    checkFile.close();
                    if (fromJSON(tempFile)) {
                        cout << "✅ DFA loaded successfully from database!" << endl;
                        displayTransitions();
                    } else {
                        cout << "❌ Failed to parse loaded DFA data!" << endl;
                    }
                    // Cleanup
                    remove(tempFile.c_str());
                } else {
                    cout << "❌ DFA with ID " << id << " not found in database!" << endl;
                }
            } else {
                cout << "❌ Error loading DFA from database!" << endl;
            }
        }
        
        bool simulate(const string& input) override {
            string currentState = startState;
            cout << "🔍 Simulating input: '" << input << "'" << endl;
            cout << "▶️  Start state: " << currentState << endl;
            
            for (char symbol : input) {
                if (alphabets.find(symbol) == alphabets.end()) {
                    cout << "❌ Symbol '" << symbol << "' not in alphabet!" << endl;
                    return false;
                }
                
                auto transition = transitions.find({currentState, symbol});
                if (transition == transitions.end()) {
                    cout << "❌ No transition from " << currentState << " with symbol " << symbol << endl;
                    return false;
                }
                
                cout << "   " << currentState << " --" << symbol << "--> " << transition->second << endl;
                currentState = transition->second;
            }
            
            bool accepted = acceptingStates.find(currentState) != acceptingStates.end();
            cout << " Final state: " << currentState;
            cout << " (" << (accepted ? "✅ ACCEPTED" : " REJECTED") << ")" << endl;
            return accepted;
        }
        
        void addTransition(const string& from, char symbol, const string& to) {
            transitions[{from, symbol}] = to;
        }
        void displayTransitions() const {
            cout << "\n Transition Table:" << endl;
            cout << "-------------------" << endl;
            for (const auto& transition : transitions) {
                if(transition.first.first ==startState ){
                    cout << transition.first.first + "*" << " --" << transition.first.second 
                        << "--> " << transition.second << endl;
                }else{
                    cout << transition.first.first << " --" << transition.first.second 
                    << "--> " << transition.second << endl;
                }
            }
            cout << endl;
        }
        // Getters for database operations
        const map<pair<string,char>, string>& getTransitions() const { return transitions; }
        const string& getStartState() const { return startState; }
        const set<string>& getAcceptingStates() const { return acceptingStates; }
        void handleInputForDFA(){
            cout << "======== Designing DFA =========" << endl;
            cout << "Enter number of states : ";
            int numState;
            cin >> numState;
            numOfStates = numState;
            for(int i = 0 ; i < numOfStates ; i++){
                string state = "q"+ to_string(i);
                addStates(state);
            }
            int numAlphabet;
            cout << "Enter number of symbols in alphabet : ";
            cin >> numAlphabet;
            numOfAlphabet = numAlphabet;
            for(int i = 0 ; i < numOfAlphabet ; i++){
                char symbol;
                cout << "Enter symbol "<< i+1 << ": ";
                cin >> symbol;
                addSymbol(symbol);
            } 
            cout << "You have " <<numOfStates << " states there're ";
            displayState(); 
            cout <<" and "<< numOfAlphabet << " symbols there're ";
            displaySymbol();
            cout <<"in your DFA."<<endl;
            string startstate;
            do{
                cout << "Enter start state : ";
                cin >> startstate;
                if(states.find(startstate) == states.end()){
                    cout << "Error : Start state must be one of the defined states."<<endl;
                }
            }while(states.find(startstate) == states.end());
            startState = startstate;
            int acceptingState;
            cout << "Enter number of accepting states : ";
            cin >> acceptingState;
            numOfAcceptingStates = acceptingState ;
            for(int i =0 ; i < numOfAcceptingStates ; i++){
                string acceptingState;
                do{
                    cout << "Enter accepting states " << i+1 << " : ";
                    cin >> acceptingState;
                    if(states.find(acceptingState) == states.end()){
                        cout << "Error: Accepting state must be one of the defined states." << endl;
                    } 
                }while(states.find(acceptingState) == states.end());  
                addAcceptingStates(acceptingState);
            }
            for(const auto& state : states){
                for(const auto& alphabet : alphabets){
                    string toState;
                    do{
                        cout << "Enter transition from state "<<state << " with symbol "<<alphabet << " to state : ";
                        cin >> toState;
                        if(states.find(toState)==states.end()){
                            cout << "Error: Transition state must be one of the defined states."<<endl;
                        }
                    }while(states.find(toState) == states.end());
                    addTransition(state, alphabet, toState);
                }
            }
            
            // ✅ ADDED: Test the DFA and offer to save to database
            cout << "\n DFA created successfully!" << endl;
            displayTransitions();
            
            // Test the DFA
            char testChoice;
            cout << "\n🧪 Do you want to test the DFA? (y/n): ";
            cin >> testChoice;
            if(testChoice == 'y' || testChoice == 'Y') {
                string testInput;
                do {
                    cout << "Enter a string to test (or 'quit' to stop): ";
                    cin >> testInput;
                    if(testInput != "quit") {
                        cout << "\n" << string(30, '-') << endl;
                        if(simulate(testInput)) {
                            cout << "🎉 ACCEPTED!" << endl;
                        } else {
                            cout << "💥 REJECTED!" << endl;
                        }
                        cout << string(30, '-') << endl;
                    }
                } while(testInput != "quit");
            }
            
            // Save to database
            char saveChoice;
            cout << "\n💾 Do you want to save this DFA to database? (y/n): ";
            cin >> saveChoice;
            if(saveChoice == 'y' || saveChoice == 'Y') {
                saveToDatabase();
            }
}
};


class NFA : public FiniteAutoMaton {
    private:
        map<pair<string,char>, set<string>> transitions;
        bool isAllowEpsilonTransitions = false; 
    public:
        // ✅ ADD: Convert NFA to JSON (similar to DFA but handles multiple transitions)
        string toJSON(const string& name) const {
            stringstream json;
            json << "{\n";
            json << "  \"name\": \"" << name << "\",\n";
            json << "  \"numOfStates\": " << numOfStates << ",\n";
            json << "  \"numOfAlphabet\": " << numOfAlphabet << ",\n";
            json << "  \"numOfAcceptingStates\": " << numOfAcceptingStates << ",\n";
            
            // States array
            json << "  \"states\": [";
            bool first_state = true;
            for (const auto& state : states) {
                if (!first_state) json << ", ";
                json << "\"" << state << "\"";
                first_state = false;
            }
            json << "],\n";
            
            // Start state
            json << "  \"startState\": \"" << startState << "\",\n";
            
            // Alphabet array
            json << "  \"alphabet\": [";
            bool first_symbol = true;
            for (const auto& symbol : alphabets) {
                if (!first_symbol) json << ", ";
                json << "\"" << string(1, symbol) << "\"";
                first_symbol = false;
            }
            json << "],\n";
            
            // Accepting states array
            json << "  \"acceptingStates\": [";
            bool first_accepting = true;
            for (const auto& state : acceptingStates) {
                if (!first_accepting) json << ", ";
                json << "\"" << state << "\"";
                first_accepting = false;
            }
            json << "],\n";
            
            // ✅ DIFFERENT: NFA transitions (multiple destinations per state-symbol)
            json << "  \"transitions\": [";
            bool first_transition = true;
            for (const auto& transition : transitions) {
                const string& fromState = transition.first.first;
                const char& symbol = transition.first.second;
                const set<string>& toStates = transition.second;
                
                // Create separate transition entry for each destination
                for (const string& toState : toStates) {
                    if (!first_transition) json << ", ";
                    json << "[\"" << fromState << "\", \"" 
                        << string(1, symbol) << "\", \"" 
                        << toState << "\"]";
                    first_transition = false;
                }
            }
            json << "]\n";
            json << "}";
            
            return json.str();
        }
        
        // Parse JSON and populate NFA
        bool fromJSON(const string& jsonFile) {
            // Similar to DFA but handle multiple transitions
            ifstream file(jsonFile);
            if (!file.is_open()) {
                cout << "❌ Failed to open JSON file: " << jsonFile << endl;
                return false;
            }
            
            string line, content;
            while (getline(file, line)) {
                content += line;
            }
            file.close();
            
            try {
                // Extract name
                size_t namePos = content.find("\"name\": \"");
                if (namePos != string::npos) {
                    namePos += 9;
                    size_t endPos = content.find("\"", namePos);
                    name = content.substr(namePos, endPos - namePos);
                }
                
                // Extract startState
                size_t startPos = content.find("\"startState\": \"");
                if (startPos != string::npos) {
                    startPos += 15;
                    size_t endPos = content.find("\"", startPos);
                    startState = content.substr(startPos, endPos - startPos);
                }
                
                return true;
            } catch (...) {
                cout << "❌ Error parsing JSON file" << endl;
                return false;
            }
        }
        
        void saveToDatabase() override {
            string nfaName;
            cout << "Enter a name for this NFA: ";
            cin >> nfaName;
            
            // Create JSON file
            string jsonData = toJSON(nfaName);
            string tempFile = "temp_nfa_save.json";
            
            ofstream jsonFile(tempFile);
            if (!jsonFile.is_open()) {
                cout << "❌ Failed to create temporary JSON file!" << endl;
                return;
            }
            jsonFile << jsonData;
            jsonFile.close();
            
            // Call Python script (same as DFA)
            string command = "python db_operation.py insert " + tempFile;
            cout << "💾 Saving NFA to database..." << endl;
            
            int result = system(command.c_str());
            
            // Cleanup temporary file
            remove(tempFile.c_str());
            
            if (result == 0) {
                cout << "✅ NFA '" << nfaName << "' saved to database successfully!" << endl;
            } else {
                cout << "❌ Failed to save NFA to database!" << endl;
            }
        }
        
        void loadFromDatabase(int id) override {
            string tempFile = "temp_nfa_load.json";
            string command = "python db_operation.py load " + to_string(id) + " " + tempFile;
            
            cout << "📂 Loading NFA from database (ID: " << id << ")..." << endl;
            
            int result = system(command.c_str());
            
            if (result == 0) {
                ifstream checkFile(tempFile);
                if (checkFile.good()) {
                    checkFile.close();
                    if (fromJSON(tempFile)) {
                        cout << "✅ NFA loaded successfully from database!" << endl;
                        displayTransitions();
                    } else {
                        cout << "❌ Failed to parse loaded NFA data!" << endl;
                    }
                    remove(tempFile.c_str());
                } else {
                    cout << "❌ NFA with ID " << id << " not found in database!" << endl;
                }
            } else {
                cout << "❌ Error loading NFA from database!" << endl;
            }
        }
        
        bool simulate(const string& input) override {
            cout << "NFA simulation not implemented yet." << endl;
            return false;
        }
        
        void addTransition(const string& from, char symbol, const string& to) {
            transitions[{from, symbol}].insert(to);
        }
        void handleInputForNFA(){
            cout << "======== Designing NFA ========="<<endl;
            bool isValid = false;
            do{
                cout << "Is this NFA have epsilon transitions?(y/n): ";
                char choice;
                cin >> choice;
                if(choice == 'y' || choice == 'n'){
                    isValid = true;
                    isAllowEpsilonTransitions = choice = 'y' ? true : false; 
                }else{
                    cout << "Error: Invalid choice. Please enter y or n."<<endl;
                }
            }while(!isValid);
            int numStates;
            do{
                cout << "Enter number of states : ";
                cin >> numStates;
                if(numStates <= 0){
                    cout << "Error: Number of states must be greater than 0."<< endl;
                }
            }while(numStates <= 0);
            numOfStates = numStates;
            string notransition = "nt";
            addStates(notransition);
            for(int i = 0 ; i < numOfStates ; i++){
                string state = "q" + to_string(i);
                addStates(state);
            }
            int numAlphabet;
            do{
                cout << "Enter number of symbols : ";
                cin >> numAlphabet;
                if(numAlphabet <= 0){
                    cout << "Error: Number of symbols must be greater than 0." << endl;
                }
            }while(numAlphabet <=0 );
            numOfAlphabet = numAlphabet;
            for(int i = 0 ; i < numOfAlphabet ; i++){
                char symbol;
                cout << "Enter symbol " << i+1 << ": ";
                cin >> symbol;
                addSymbol(symbol);
            }
            cout << "You have " << numOfStates << " states there're ";
            displayState();
            cout << " and " << numOfAlphabet << " symbols there're ";
            displaySymbol();
            cout << "in your NFA." << endl;
            string state_state;
            do{
                cout << "Enter start state : ";
                cin >> state_state;
                if(states.find(state_state)== states.end()){
                    cout << "Error: Start state must be one of the defined states." << endl;
                }
            }while(states.find(state_state) == states.end());
            startState = state_state;
            int numOfAcc;
            do{
                cout << "Enter number of accepting states : ";
                cin >> numOfAcc;
                if(numOfAcc <= 0){
                    cout << "Error: Number of accepting states must be greater than 0."<< endl;
                }
            }while(numOfAcc <= 0);
            numOfAcceptingStates = numOfAcc;
            for(int i = 0 ;i < numOfAcceptingStates ; i++){
                string acceptingState;
                do{
                    cout << "Enter accepting state " << i+1 << ": ";
                    cin >> acceptingState;
                    if(states.find(acceptingState) == states.end()){
                        cout << "Error: Accepting state must be one of the defined states." << endl;
                    }
                }while(states.find(acceptingState) == states.end());
                addAcceptingStates(acceptingState);
            }
            for(const auto& state : states){
                for(const auto& alphabet : alphabets){
                    char choice;
                    do{
                        string toState;
                        do{
                            cout << "Enter transition from state "<<state  << " with symbol " << alphabet << " to state(for no transition you can enter (nf) : ";
                            cin >> toState;
                            if(states.find(toState) == states.end()){
                                cout << "Error: Transition state must be one of the defined states." << endl;
                            }
                        }while(states.find(toState) == states.end());
                        addTransition(state, alphabet, toState);
                        do{
                            cout << "Is there another transition from state "<<state<<" with symbol "<<alphabet<<"? (y/n): ";
                            cin >> choice;
                            if(choice != 'y' && choice !='n'){
                                cout << "Error: Invalid choice. Please enter y or n."<<endl;
                            }
                        }while(choice != 'y' && choice != 'n');
                        
                    }while(choice == 'y');
                }
            }
            cout << "\nNFA created successfully!"<<endl;
            char isSave;
            do{
                cout << "Do you want to save this NFA to Database? (y/n): ";
                cin >> isSave;
                if(isSave == 'y'){
                    saveToDatabase();
                }
                if(isSave !='y' && isSave != 'n' ){
                    cout << "Error: Invalid choice. Please enter y or n." << endl;
                }
            }while(isSave != 'y' && isSave != 'n');
        }

        // ✅ ADD: Display NFA transitions (different from DFA)
        void displayTransitions() const {
            cout << "\n📋 NFA Transition Table:" << endl;
            cout << "------------------------" << endl;
            for (const auto& transition : transitions) {
                const string& fromState = transition.first.first;
                const char& symbol = transition.first.second;
                const set<string>& toStates = transition.second;
                
                cout << fromState << " --" << symbol << "--> {";
                bool first = true;
                for (const string& toState : toStates) {
                    if (!first) cout << ", ";
                    cout << toState;
                    first = false;
                }
                cout << "}" << endl;
            }
            cout << endl;
        }
};

void menu(){
    cout << "--------- Finite Automaton Menu ---------" << endl;
    cout << " 1. Design a FA"<<endl;
    cout << " 2. Test a FA from Database"<<endl;
    cout << " 3. Convert NFA to DFA"<<endl;
    cout << " 4. Check type of FA"<<endl;
    cout << " 5. Minimize DFA"<<endl;
    cout << " 0. exit "<<endl;
    cout << "please enter your choice: "<<endl;
}

void menuForDesignFA(){
    cout << "--------- Design Finite Automaton ---------" << endl;
    cout << " 1. Create DFA" << endl;
    cout << " 2. Create NFA" << endl;
    cout << " 0. Back to main menu" << endl;
    cout << "please enter your choice: " << endl;
}

void handleUserInputForMenu(){
    int choice;
    do{
        menu();
        cin >> choice;
        switch(choice){
            case 1 : {
                int choiceForDesignFa;
                do{
                    menuForDesignFA();
                    cin >> choiceForDesignFa;
                    switch(choiceForDesignFa){
                        case 1 :{ 
                            DFA dfa;
                            dfa.handleInputForDFA();
                        }
                        break;
                        case 2 :{
                            NFA nfa;
                            nfa.handleInputForNFA();
                        }
                        break;
                        case 0 :
                            cout << "Returning to main menu." << endl;
                            break;     
                        default :
                            cout << "invalid choice please try again." << endl;
                            break;
                    }  
                }while(choiceForDesignFa != 0);
            }
            break;
            case 2 : {
                // ✅ ADDED: Test FA from database
                cout << "=== Test a FA from Database ===" << endl;
                cout << "Enter DFA ID to load: ";
                int dfaId;
                cin >> dfaId;
                
                DFA dfa;
                dfa.loadFromDatabase(dfaId);
                
                // Test the loaded DFA
                string testInput;
                do {
                    cout << "\nEnter string to test (or 'quit' to stop): ";
                    cin >> testInput;
                    if(testInput != "quit") {
                        cout << "\n" << string(40, '=') << endl;
                        if(dfa.simulate(testInput)) {
                            cout << "🎉 Result: ACCEPTED!" << endl;
                        } else {
                            cout << "💥 Result: REJECTED!" << endl;
                        }
                        cout << string(40, '=') << endl;
                    }
                } while(testInput != "quit");
            }
            break;
            case 3 :{ 
                cout << "NFA to DFA conversion not implemented yet." << endl;
            }
            break;
            case 4 :{
                cout << "FA type checking not implemented yet." << endl;
            }
            break;
            case 5 : {
                cout << "DFA minimization not implemented yet." << endl;
            }
            break;
            case 0 :
                cout << "👋 Exiting the program." << endl;
                return;    
            default:
                cout << "Invalid choice, please try again." << endl;
            break;        
        }
    }while(choice != 0);
}

int main(){
    handleUserInputForMenu();
    return 0;
}