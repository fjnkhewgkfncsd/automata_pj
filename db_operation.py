from db_config import db_config
import mysql.connector
import sys
import json

def insert_fa(fa,db_config):
    db = None
    cursor = None
    try:
        db = mysql.connector.connect(**db_config)
        cursor = db.cursor()
        db.start_transaction()

        cursor.execute("""
        insert into Automata (name, num_of_states, num_of_alphabet_symbols, num_of_accepting_states) values (%s, %s, %s, %s)
        """,(fa["name"],
        fa["numOfStates"],
        fa["numOfAlphabet"],
        fa["numOfAcceptingStates"]
        ))
        automaton_id = cursor.lastrowid

        state_id_map = {}
        for state in fa["states"]:
            cursor.execute("insert into States (automaton_id,state_name) values (%s,%s)",(automaton_id,state))
            state_id_map[state] = cursor.lastrowid
        
        cursor.execute("update automata set start_state_id = %s where automaton_id = %s",
                        (state_id_map[fa["startState"]], automaton_id))
        symbol_id_map = {}
        for symbol in fa["alphabet"]:
            cursor.execute("insert into AlphabetSymbols (automaton_id,symbol_value) values (%s,%s)",
                            (automaton_id, symbol))
            symbol_id_map[symbol] = cursor.lastrowid
            
        for acc in fa["acceptingStates"]:
            cursor.execute("insert into AcceptingStates (automaton_id,state_id) values (%s,%s)",
                            (automaton_id, state_id_map[acc])) 
            
        for (src, sym, dst) in fa["transitions"]:
            cursor.execute("""
            insert into Transitions (automaton_id, current_state_id, symbol_id, next_state_id)
                                        values (%s,%s,%s,%s)
            """,(
                automaton_id,
                state_id_map[src],
                symbol_id_map[sym],
                state_id_map[dst]
            )
            )
        db.commit()
        return automaton_id  # ✅ Return the automaton ID on success
    except mysql.connector.Error as err:
        db.rollback()
        print(f" Error inserting FA '{fa['name']}': {err}")
    finally:
        if cursor:
            cursor.close()
        if db:    
            db.close()

def load_fa(automaton_id, db_config):
    try:
        db = mysql.connector.connect(**db_config)
        cursor = db.cursor()

        # 1. Load main automaton info with start state
        cursor.execute("""
        SELECT a.automaton_id, a.name, a.num_of_states, a.num_of_alphabet_symbols, 
                a.num_of_accepting_states, s.state_name as start_state
        FROM Automata a 
        LEFT JOIN States s ON a.start_state_id = s.state_id 
        WHERE a.automaton_id = %s
        """, (automaton_id,))  # ✅ Fixed: Added comma for tuple
        
        automaton = cursor.fetchone()
        if not automaton:
            print(f" Automaton with ID {automaton_id} not found.")
            return None

        # Extract main info
        fa_data = {
            "id": automaton[0],
            "name": automaton[1],
            "numOfStates": automaton[2],
            "numOfAlphabet": automaton[3],
            "numOfAcceptingStates": automaton[4],
            "startState": automaton[5]
        }

        # 2. Load all states
        cursor.execute("""
        SELECT s.state_name 
        FROM States s 
        WHERE s.automaton_id = %s
        ORDER BY s.state_id
        """, (automaton_id,))
        
        fa_data["states"] = [row[0] for row in cursor.fetchall()]

        # 3. Load alphabet symbols
        cursor.execute("""
        SELECT al.symbol_value 
        FROM AlphabetSymbols al 
        WHERE al.automaton_id = %s
        ORDER BY al.symbol_id
        """, (automaton_id,))
        
        fa_data["alphabet"] = [row[0] for row in cursor.fetchall()]

        # 4. Load accepting states using JOIN
        cursor.execute("""
        SELECT s.state_name 
        FROM AcceptingStates acc 
        JOIN States s ON acc.state_id = s.state_id 
        WHERE acc.automaton_id = %s
        ORDER BY s.state_name
        """, (automaton_id,))
        
        fa_data["acceptingStates"] = [row[0] for row in cursor.fetchall()]

        # 5. Load transitions using multiple JOINs
        cursor.execute("""
        SELECT s1.state_name as from_state, 
                al.symbol_value as symbol, 
                s2.state_name as to_state
        FROM Transitions t
        JOIN States s1 ON t.current_state_id = s1.state_id
        JOIN AlphabetSymbols al ON t.symbol_id = al.symbol_id  
        JOIN States s2 ON t.next_state_id = s2.state_id
        WHERE t.automaton_id = %s
        ORDER BY s1.state_name, al.symbol_value
        """, (automaton_id,))
        
        fa_data["transitions"] = [(row[0], row[1], row[2]) for row in cursor.fetchall()]

        print(f" FA '{fa_data['name']}' loaded successfully with {len(fa_data['transitions'])} transitions.")
        return fa_data

    except mysql.connector.Error as err:
        print(f" Database error: {err}")
        return None
    finally:
        if 'cursor' in locals():
            cursor.close()
        if 'db' in locals():
            db.close()

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python db_operation.py <command> [args]")
        sys.exit(1)

    command = sys.argv[1]

    if command == "insert":
        if len(sys.argv) != 3:
            print("Usage: python db_operation.py insert <json_file>")
            sys.exit(1)
        
        json_file = sys.argv[2]
        try:
            with open(json_file, 'r') as f:
                fa_data = json.load(f)
            
            result = insert_fa(fa_data, db_config)
            if result is not None:
                print("SUCCESS")
                sys.exit(0)
            else:
                print("FAILED")
                sys.exit(1)
        except Exception as e:
            print(f"ERROR: {e}")
    elif command == "load":
        if len(sys.argv) != 4:
            print("Usage: python db_operation.py load <automaton_id> <output_file>")
            sys.exit(1)
        
        automaton_id = int(sys.argv[2])
        output_file = sys.argv[3]
        
        try:
            fa_data = load_fa(automaton_id, db_config)
            if fa_data:
                with open(output_file, 'w') as f:
                    json.dump(fa_data, f, indent=2)
                print("SUCCESS")
            else:
                print("NOT_FOUND")
        except Exception as e:
            print(f"ERROR: {e}")

    else:
        print("Unknown command. Use 'insert' or 'load'")
        sys.exit(1)



