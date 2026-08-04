/*
 * ============================================================================
 *  HomePaws - A Cozy Virtual Pet & Household Simulator
 * ============================================================================
 *  Course   : Object Oriented Programming (First Year BCT, Tribhuvan University)
 *  Language : C++17
 *
 *  Description:
 *      HomePaws lets the user build a household of people who take care of
 *      multiple virtual pets (Dogs and Cats). Pets have needs (hunger, thirst,
 *      health, happiness, energy, hygiene, affection) that change over time
 *      and in response to activities like feeding, playing, walking, training,
 *      brushing, bathing, sleeping and vet visits. The simulation supports
 *      random events, achievements, statistics tracking and save/load via
 *      plain text file handling.
 *
 *  OOP concepts demonstrated:
 *      - Encapsulation   : private/protected data members with public getters/setters
 *      - Constructors    : default & parameterized constructors across all classes
 *      - Inheritance     : Pet -> Dog, Pet -> Cat
 *      - Polymorphism    : virtual functions overridden by Dog/Cat, base pointers
 *      - Composition     : Household "has-a" vector<Person> and vector<unique_ptr<Pet>>
 *      - STL             : vector, string, sstream, algorithm
 *      - File handling   : ifstream/ofstream based save & load
 *      - Exceptions      : custom HomePawsException used throughout
 * ============================================================================
 */

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <memory>
#include <random>
#include <ctime>
#include <iomanip>
#include <algorithm>
#include <stdexcept>
#include <limits>
#include <cstdlib>

using namespace std;

/* ============================================================================
 *  CUSTOM EXCEPTION
 * ============================================================================
 */
class HomePawsException : public runtime_error {
public:
    explicit HomePawsException(const string& message) : runtime_error(message) {}
};

/* ============================================================================
 *  UTILITY NAMESPACE - shared helper functions (input validation, RNG, etc.)
 * ============================================================================
 */
namespace Utility {

    // Provides a single shared random engine seeded once at program start.
    inline mt19937& rng() {
        static mt19937 engine(static_cast<unsigned int>(time(nullptr)));
        return engine;
    }

    // Returns a random integer in the inclusive range [lo, hi].
    inline int randomInt(int lo, int hi) {
        uniform_int_distribution<int> dist(lo, hi);
        return dist(rng());
    }

    // Clamps an integer value between lo and hi (used to keep pet stats in 0-100).
    inline int clampInt(int value, int lo = 0, int hi = 100) {
        if (value < lo) return lo;
        if (value > hi) return hi;
        return value;
    }

    // Removes leading/trailing whitespace from a string.
    inline string trim(const string& s) {
        size_t start = s.find_first_not_of(" \t\r\n");
        if (start == string::npos) return "";
        size_t end = s.find_last_not_of(" \t\r\n");
        return s.substr(start, end - start + 1);
    }

    // Clears any leftover characters/error flags on cin so future reads work correctly.
    inline void clearInputStream() {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }

    // Pauses program flow until the user presses Enter.
    inline void pause() {
        cout << "\nPress Enter to continue...";
        if (cin.get() == EOF) {
            cout << "\nInput stream closed. Exiting HomePaws. Goodbye!\n";
            exit(0);
        }
    }

    // Prompts repeatedly until the user enters a valid integer within [lo, hi].
    // If the input stream is closed/exhausted (EOF), the program exits gracefully
    // instead of looping forever - important for robustness.
    inline int getValidatedInt(const string& prompt, int lo, int hi) {
        int value;
        while (true) {
            cout << prompt;
            if (cin >> value) {
                clearInputStream();
                if (value >= lo && value <= hi) return value;
                cout << "Invalid input. Please enter a whole number between "
                     << lo << " and " << hi << ".\n";
                continue;
            }
            if (cin.eof()) {
                cout << "\nInput stream closed. Exiting HomePaws. Goodbye!\n";
                exit(0);
            }
            cout << "Invalid input. Please enter a whole number between "
                 << lo << " and " << hi << ".\n";
            clearInputStream();
        }
    }

    // Prompts repeatedly until the user enters a non-empty line (unless allowEmpty is true).
    inline string getValidatedLine(const string& prompt, bool allowEmpty = false) {
        string input;
        while (true) {
            cout << prompt;
            if (!getline(cin, input)) {
                cout << "\nInput stream closed. Exiting HomePaws. Goodbye!\n";
                exit(0);
            }
            input = trim(input);
            if (!input.empty() || allowEmpty) return input;
            cout << "This field cannot be empty. Please try again.\n";
        }
    }

    // Splits a string by a delimiter character into a vector of tokens.
    inline vector<string> split(const string& text, char delimiter) {
        vector<string> tokens;
        stringstream ss(text);
        string token;
        while (getline(ss, token, delimiter)) {
            tokens.push_back(token);
        }
        return tokens;
    }

    // Prints a decorative section header for menus.
    inline void printHeader(const string& title) {
        cout << "\n============================================================\n";
        cout << "  " << title << "\n";
        cout << "============================================================\n";
    }
}

/* ============================================================================
 *  ENUMERATIONS: Gender & Personality  (with string conversion helpers)
 * ============================================================================
 */
enum class Gender { Male, Female };

inline string genderToString(Gender g) {
    return (g == Gender::Male) ? "Male" : "Female";
}

inline Gender stringToGender(const string& s) {
    return (s == "Male") ? Gender::Male : Gender::Female;
}

enum class Personality { Playful, Lazy, Friendly, Shy, Curious, Protective };

inline string personalityToString(Personality p) {
    switch (p) {
        case Personality::Playful:    return "Playful";
        case Personality::Lazy:       return "Lazy";
        case Personality::Friendly:   return "Friendly";
        case Personality::Shy:        return "Shy";
        case Personality::Curious:    return "Curious";
        case Personality::Protective: return "Protective";
    }
    return "Friendly";
}

inline Personality stringToPersonality(const string& s) {
    if (s == "Playful")    return Personality::Playful;
    if (s == "Lazy")       return Personality::Lazy;
    if (s == "Friendly")   return Personality::Friendly;
    if (s == "Shy")        return Personality::Shy;
    if (s == "Curious")    return Personality::Curious;
    if (s == "Protective") return Personality::Protective;
    return Personality::Friendly;
}

// Common dog and cat breeds offered to the user during adoption.
inline vector<string> dogBreedList() {
    return {
        "Labrador Retriever", "German Shepherd", "Golden Retriever", "Bulldog",
        "Poodle", "Beagle", "Rottweiler", "Yorkshire Terrier", "Boxer",
        "Dachshund", "Siberian Husky", "Great Dane", "Doberman Pinscher",
        "Shih Tzu", "Chihuahua", "Pomeranian", "Border Collie",
        "Australian Shepherd", "Cocker Spaniel", "Pug", "Nepali Kukur (Mixed Breed)"
    };
}

inline vector<string> catBreedList() {
    return {
        "Persian", "Maine Coon", "Siamese", "Ragdoll", "Bengal", "Sphynx",
        "British Shorthair", "Abyssinian", "Scottish Fold", "Russian Blue",
        "American Shorthair", "Norwegian Forest Cat", "Himalayan", "Devon Rex",
        "Burmese", "Turkish Angora", "Manx", "Birman", "Exotic Shorthair",
        "Nepali Local Cat"
    };
}

/* ============================================================================
 *  CLASS: Person
 *  Represents a household member. Demonstrates basic encapsulation.
 * ============================================================================
 */
class Person {
private:
    string name;
    int age;
    string role; // e.g. Parent, Child, Guardian, Roommate, Grandparent

public:
    // Default constructor
    Person() : name("Unknown"), age(0), role("Member") {}

    // Parameterized constructor
    Person(const string& personName, int personAge, const string& personRole) {
        setName(personName);
        setAge(personAge);
        setRole(personRole);
    }

    // ---- Getters ----
    string getName() const { return name; }
    int getAge() const { return age; }
    string getRole() const { return role; }

    // ---- Setters (with validation -> encapsulation) ----
    void setName(const string& n) {
        if (Utility::trim(n).empty()) throw HomePawsException("Person name cannot be empty.");
        name = n;
    }

    void setAge(int a) {
        if (a < 0 || a > 130) throw HomePawsException("Person age must be between 0 and 130.");
        age = a;
    }

    void setRole(const string& r) {
        role = Utility::trim(r).empty() ? "Member" : r;
    }

    // Displays this person's information.
    void display() const {
        cout << "  - " << left << setw(18) << name
             << " | Age: " << setw(4) << age
             << " | Role: " << role << "\n";
    }

    // Converts this person into a pipe-delimited string for saving to file.
    string serialize() const {
        return name + "|" + to_string(age) + "|" + role;
    }

    // Reconstructs a Person object from a pipe-delimited line loaded from file.
    static Person deserialize(const string& line) {
        vector<string> parts = Utility::split(line, '|');
        if (parts.size() < 3) throw HomePawsException("Corrupted member record in save file.");
        return Person(parts[0], stoi(parts[1]), parts[2]);
    }
};

/* ============================================================================
 *  CLASS: Pet (ABSTRACT BASE CLASS)
 *  Demonstrates encapsulation, constructors, and polymorphism via virtual
 *  functions. Dog and Cat inherit from this class and override behavior.
 * ============================================================================
 */
class Pet {
protected:
    string name;
    string breed;
    Gender gender;
    Personality personality;
    int ageInDays;
    string color;
    string favoriteFood;
    string favoriteToy;
    string favoriteOwner;   // name of the household member this pet likes most

    // Core stats, kept within the range [0, 100]
    int hunger;
    int thirst;
    int health;
    int happiness;
    int energy;
    int hygiene;
    int affection;
    string mood;            // derived text description based on overall stats

public:
    // Parameterized constructor - every pet must be created with full details.
    Pet(const string& petName, const string& petBreed, Gender petGender,
        Personality petPersonality, const string& petColor,
        const string& petFavoriteFood, const string& petFavoriteToy,
        const string& petFavoriteOwner)
        : name(petName), breed(petBreed), gender(petGender), personality(petPersonality),
          ageInDays(0), color(petColor), favoriteFood(petFavoriteFood),
          favoriteToy(petFavoriteToy), favoriteOwner(petFavoriteOwner),
          hunger(60), thirst(60), health(100), happiness(70), energy(80),
          hygiene(80), affection(50), mood("Content")
    {
        if (Utility::trim(name).empty()) throw HomePawsException("Pet name cannot be empty.");
    }

    // Virtual destructor - essential for correct cleanup through base pointers.
    virtual ~Pet() = default;

    /* ---------------- Pure virtual functions (must be overridden) ---------------- */
    virtual string getSpeciesName() const = 0;      // "Dog" or "Cat"
    virtual string serializeExtra() const = 0;      // species-specific save data

    /* ---------------- Virtual activity functions (overridable) ------------------- */
    virtual void feed();
    virtual void giveWater();
    virtual void play();
    virtual void train();
    virtual void brush();
    virtual void bathe();
    virtual void sleep();
    virtual void visitVet();
    virtual void walk();              // base version: most pets don't "walk" formally
    virtual void applyDailyDecay();   // called once per simulated day
    virtual void displayInfo() const;
    virtual void displaySummary() const;

    /* ---------------- Shared (non-virtual) behavior ------------------------------- */
    void agePetByOneDay() { ageInDays++; }
    void updateMood();
    void renamePet(const string& newName) {
        if (Utility::trim(newName).empty()) throw HomePawsException("New name cannot be empty.");
        name = newName;
    }

    /* ---------------- Getters ---------------- */
    string getName() const { return name; }
    string getBreed() const { return breed; }
    Gender getGender() const { return gender; }
    Personality getPersonality() const { return personality; }
    int getAgeInDays() const { return ageInDays; }
    double getAgeInYears() const { return ageInDays / 365.0; }
    string getColor() const { return color; }
    string getFavoriteFood() const { return favoriteFood; }
    string getFavoriteToy() const { return favoriteToy; }
    string getFavoriteOwner() const { return favoriteOwner; }
    int getHunger() const { return hunger; }
    int getThirst() const { return thirst; }
    int getHealth() const { return health; }
    int getHappiness() const { return happiness; }
    int getEnergy() const { return energy; }
    int getHygiene() const { return hygiene; }
    int getAffection() const { return affection; }
    string getMood() const { return mood; }

    /* ---------------- Setters ---------------- */
    void setFavoriteOwner(const string& owner) { favoriteOwner = owner; }
    void setAgeInDays(int days) { ageInDays = (days < 0) ? 0 : days; }
    void setRawStats(int h, int t, int he, int ha, int e, int hy, int a) {
        hunger = Utility::clampInt(h);
        thirst = Utility::clampInt(t);
        health = Utility::clampInt(he);
        happiness = Utility::clampInt(ha);
        energy = Utility::clampInt(e);
        hygiene = Utility::clampInt(hy);
        affection = Utility::clampInt(a);
        updateMood();
    }

    // Base part of the serialized record shared by all pet types.
    string serializeBase() const {
        stringstream ss;
        ss << name << "|" << breed << "|" << genderToString(gender) << "|"
           << personalityToString(personality) << "|" << ageInDays << "|" << color << "|"
           << favoriteFood << "|" << favoriteToy << "|" << favoriteOwner << "|"
           << hunger << "|" << thirst << "|" << health << "|" << happiness << "|"
           << energy << "|" << hygiene << "|" << affection;
        return ss.str();
    }

protected:
    // Adjusts a base happiness/stat gain according to this pet's personality trait.
    // Demonstrates how personality influences behavior across all activities.
    int personalityModifier(int baseAmount) const {
        switch (personality) {
            case Personality::Playful:    return baseAmount + 10;
            case Personality::Lazy:       return baseAmount - 8;
            case Personality::Friendly:   return baseAmount + 5;
            case Personality::Shy:        return baseAmount - 5;
            case Personality::Curious:    return baseAmount + 6;
            case Personality::Protective: return baseAmount;
        }
        return baseAmount;
    }
};

/* ---------------------------- Pet method definitions ---------------------------- */

void Pet::feed() {
    hunger = Utility::clampInt(hunger + 35);
    happiness = Utility::clampInt(happiness + personalityModifier(5));
    if (personality == Personality::Lazy) energy = Utility::clampInt(energy + 5);
    updateMood();
    cout << name << " enjoyed a meal of " << favoriteFood << "!\n";
}

void Pet::giveWater() {
    thirst = Utility::clampInt(thirst + 35);
    happiness = Utility::clampInt(happiness + 2);
    updateMood();
    cout << name << " drank some fresh water.\n";
}

void Pet::play() {
    happiness = Utility::clampInt(happiness + personalityModifier(20));
    energy = Utility::clampInt(energy - (personality == Personality::Lazy ? 20 : 15));
    hygiene = Utility::clampInt(hygiene - 5);
    affection = Utility::clampInt(affection + 8);
    hunger = Utility::clampInt(hunger - 5);
    updateMood();
    cout << name << " had a great time playing with " << favoriteToy << "!\n";
}

void Pet::train() {
    happiness = Utility::clampInt(happiness + personalityModifier(8));
    energy = Utility::clampInt(energy - 15);
    affection = Utility::clampInt(affection + 5);
    updateMood();
    cout << name << " practiced some training exercises.\n";
}

void Pet::brush() {
    hygiene = Utility::clampInt(hygiene + 20);
    affection = Utility::clampInt(affection + 8);
    happiness = Utility::clampInt(happiness + 5);
    updateMood();
    cout << name << " looks neat and tidy after a good brushing!\n";
}

void Pet::bathe() {
    hygiene = 100;
    happiness = Utility::clampInt(happiness - (personality == Personality::Shy ? 15 : 5));
    updateMood();
    cout << name << " is squeaky clean after a bath.\n";
}

void Pet::sleep() {
    energy = 100;
    happiness = Utility::clampInt(happiness + 5);
    hunger = Utility::clampInt(hunger - 8);
    thirst = Utility::clampInt(thirst - 8);
    updateMood();
    cout << name << " took a restful nap and feels recharged.\n";
}

void Pet::visitVet() {
    health = 100;
    happiness = Utility::clampInt(happiness - (personality == Personality::Protective ? 3 : 10));
    updateMood();
    cout << name << " had a checkup at the vet and is now in perfect health.\n";
}

void Pet::walk() {
    // Base implementation: generic pets get a small mood boost from fresh air
    // but this is overridden meaningfully by Dog.
    happiness = Utility::clampInt(happiness + 3);
    updateMood();
    cout << name << " doesn't really go on formal walks, but enjoyed some fresh air.\n";
}

void Pet::applyDailyDecay() {
    hunger = Utility::clampInt(hunger - 15);
    thirst = Utility::clampInt(thirst - 15);
    energy = Utility::clampInt(energy - 10);
    hygiene = Utility::clampInt(hygiene - 10);
    happiness = Utility::clampInt(happiness - 8);
    affection = Utility::clampInt(affection - 3);

    // Neglecting basic needs hurts health.
    if (hunger < 20 || thirst < 20) health = Utility::clampInt(health - 12);
    if (hygiene < 15) health = Utility::clampInt(health - 5);
    if (health < 30) happiness = Utility::clampInt(happiness - 5);

    updateMood();
}

void Pet::updateMood() {
    int average = (happiness + health + energy + affection) / 4;
    if (average >= 80) mood = "Ecstatic";
    else if (average >= 60) mood = "Happy";
    else if (average >= 40) mood = "Content";
    else if (average >= 20) mood = "Sad";
    else mood = "Miserable";
}

void Pet::displayInfo() const {
    cout << "\n------------------------------------------------------------\n";
    cout << " " << getSpeciesName() << ": " << name << " (" << breed << ")\n";
    cout << "------------------------------------------------------------\n";
    cout << " Gender      : " << genderToString(gender) << "\n";
    cout << " Age         : " << fixed << setprecision(2) << getAgeInYears()
         << " years (" << ageInDays << " days)\n";
    cout << " Color       : " << color << "\n";
    cout << " Personality : " << personalityToString(personality) << "\n";
    cout << " Fav. Food   : " << favoriteFood << "\n";
    cout << " Fav. Toy    : " << favoriteToy << "\n";
    cout << " Fav. Owner  : " << (favoriteOwner.empty() ? "None yet" : favoriteOwner) << "\n";
    cout << " Mood        : " << mood << "\n";
    cout << "------------------------------------------------------------\n";
    cout << " Hunger    : " << setw(3) << hunger    << " / 100\n";
    cout << " Thirst    : " << setw(3) << thirst    << " / 100\n";
    cout << " Health    : " << setw(3) << health    << " / 100\n";
    cout << " Happiness : " << setw(3) << happiness << " / 100\n";
    cout << " Energy    : " << setw(3) << energy    << " / 100\n";
    cout << " Hygiene   : " << setw(3) << hygiene   << " / 100\n";
    cout << " Affection : " << setw(3) << affection << " / 100\n";
    cout << "------------------------------------------------------------\n";
}

void Pet::displaySummary() const {
    cout << "  - " << left << setw(12) << name
         << " | " << setw(6) << getSpeciesName()
         << " | " << setw(20) << breed
         << " | Mood: " << setw(10) << mood
         << " | Health: " << health << "\n";
}

/* ============================================================================
 *  CLASS: Dog  (inherits from Pet)
 *  Adds dog-specific data (trained status, walk count) and overrides several
 *  virtual functions to give dogs their own behavior - polymorphism.
 * ============================================================================
 */
class Dog : public Pet {
private:
    bool trained;
    int walkCount;

public:
    Dog(const string& petName, const string& petBreed, Gender petGender,
        Personality petPersonality, const string& petColor,
        const string& petFavoriteFood, const string& petFavoriteToy,
        const string& petFavoriteOwner, bool startTrained = false, int startWalkCount = 0)
        : Pet(petName, petBreed, petGender, petPersonality, petColor,
              petFavoriteFood, petFavoriteToy, petFavoriteOwner),
          trained(startTrained), walkCount(startWalkCount) {}

    string getSpeciesName() const override { return "Dog"; }

    // Dogs love proper walks - override gives a much richer effect than the base class.
    void walk() override {
        happiness = Utility::clampInt(happiness + personalityModifier(20));
        energy = Utility::clampInt(energy - 18);
        hygiene = Utility::clampInt(hygiene - 12);
        hunger = Utility::clampInt(hunger - 10);
        affection = Utility::clampInt(affection + 5);
        walkCount++;
        updateMood();
        cout << name << " the dog had a wonderful walk outside! (Total walks: " << walkCount << ")\n";
    }

    // Dogs can become "trained" through repeated training sessions.
    void train() override {
        Pet::train(); // reuse base stat changes, then add dog-specific behavior
        int successChance = (personality == Personality::Curious || personality == Personality::Playful) ? 70 : 40;
        if (!trained && Utility::randomInt(1, 100) <= successChance) {
            trained = true;
            cout << name << " learned a new trick during training! Good dog!\n";
        } else {
            cout << name << " practiced obedience training.\n";
        }
    }

    bool isTrained() const { return trained; }
    int getWalkCount() const { return walkCount; }

    string serializeExtra() const override {
        return string(trained ? "1" : "0") + "," + to_string(walkCount);
    }
};

/* ============================================================================
 *  CLASS: Cat  (inherits from Pet)
 *  Cats behave differently: they dislike baths, self-groom (slower hygiene
 *  decay), and don't go on formal walks - another example of polymorphism.
 * ============================================================================
 */
class Cat : public Pet {
private:
    bool indoorOnly;

public:
    Cat(const string& petName, const string& petBreed, Gender petGender,
        Personality petPersonality, const string& petColor,
        const string& petFavoriteFood, const string& petFavoriteToy,
        const string& petFavoriteOwner, bool startIndoorOnly = true)
        : Pet(petName, petBreed, petGender, petPersonality, petColor,
              petFavoriteFood, petFavoriteToy, petFavoriteOwner),
          indoorOnly(startIndoorOnly) {}

    string getSpeciesName() const override { return "Cat"; }

    // Cats famously dislike water - override the base bathe() behavior.
    void bathe() override {
        hygiene = 100;
        happiness = Utility::clampInt(happiness - 15);
        affection = Utility::clampInt(affection - 5);
        updateMood();
        cout << name << " the cat is NOT happy about that bath, but is very clean now!\n";
    }

    // Cats self-groom, so their hygiene decays slower than dogs.
    void applyDailyDecay() override {
        Pet::applyDailyDecay();
        hygiene = Utility::clampInt(hygiene + 8); // partially offsets base decay
        updateMood();
    }

    // Cats don't go on leash walks - a light indoor stroll instead.
    void walk() override {
        happiness = Utility::clampInt(happiness + personalityModifier(6));
        energy = Utility::clampInt(energy - 5);
        updateMood();
        cout << name << " the cat prowled around the house instead of a proper walk.\n";
    }

    bool isIndoorOnly() const { return indoorOnly; }

    string serializeExtra() const override {
        return string(indoorOnly ? "1" : "0");
    }
};

/* ============================================================================
 *  STRUCT: Statistics
 *  Tracks household-wide lifetime counters. Plain struct kept simple since it
 *  is pure data owned/managed by Household (composition).
 * ============================================================================
 */
struct Statistics {
    int mealsServed   = 0;
    int walksTaken    = 0;
    int gamesPlayed   = 0;
    int bathsGiven    = 0;
    int vetVisits     = 0;
    int waterGiven    = 0;
    int trainSessions = 0;
    int brushSessions = 0;
    int sleepSessions = 0;

    int totalActivities() const {
        return mealsServed + walksTaken + gamesPlayed + bathsGiven + vetVisits +
               waterGiven + trainSessions + brushSessions + sleepSessions;
    }

    string serialize() const {
        stringstream ss;
        ss << mealsServed << "," << walksTaken << "," << gamesPlayed << ","
           << bathsGiven << "," << vetVisits << "," << waterGiven << ","
           << trainSessions << "," << brushSessions << "," << sleepSessions;
        return ss.str();
    }

    static Statistics deserialize(const string& line) {
        vector<string> parts = Utility::split(line, ',');
        Statistics s;
        if (parts.size() < 9) throw HomePawsException("Corrupted statistics record in save file.");
        s.mealsServed   = stoi(parts[0]);
        s.walksTaken    = stoi(parts[1]);
        s.gamesPlayed   = stoi(parts[2]);
        s.bathsGiven    = stoi(parts[3]);
        s.vetVisits     = stoi(parts[4]);
        s.waterGiven    = stoi(parts[5]);
        s.trainSessions = stoi(parts[6]);
        s.brushSessions = stoi(parts[7]);
        s.sleepSessions = stoi(parts[8]);
        return s;
    }
};

/* ============================================================================
 *  STRUCT: Achievement
 *  Simple record representing an unlockable milestone.
 * ============================================================================
 */
struct Achievement {
    string name;
    string description;
    bool unlocked = false;
};

/* ============================================================================
 *  CLASS: Household
 *  The central class. Uses COMPOSITION - a Household "has-a" collection of
 *  Person objects and "has-a" collection of Pet objects (via unique_ptr for
 *  correct polymorphic ownership). Owns Statistics and Achievements too.
 * ============================================================================
 */
class Household {
private:
    string householdName;
    vector<Person> members;
    vector<unique_ptr<Pet>> pets;
    Statistics stats;
    vector<Achievement> achievements;
    int totalDays;
    int currentHour; // 0-23, simulated clock within a day

public:
    explicit Household(const string& name) : householdName(name), totalDays(1), currentHour(8) {
        if (Utility::trim(name).empty()) throw HomePawsException("Household name cannot be empty.");
        initAchievements();
    }

    void initAchievements();

    /* ---------------- Household member management ---------------- */
    void addMember(const Person& person);
    void editMember(int index, const string& newName, int newAge, const string& newRole);
    void removeMember(int index);
    void listMembers() const;
    int memberCount() const { return static_cast<int>(members.size()); }
    vector<Person>& getMembers() { return members; }

    /* ---------------- Pet management ---------------- */
    void adoptPet(unique_ptr<Pet> newPet);
    void removePet(const string& petName);
    Pet* findPet(const string& petName); // linear search, case-insensitive
    void listPets() const;
    void viewPet(const string& petName) const;
    int petCount() const { return static_cast<int>(pets.size()); }
    vector<unique_ptr<Pet>>& getPets() { return pets; }

    /* ---------------- Pet care activities ---------------- */
    void feedPet(const string& petName);
    void waterPet(const string& petName);
    void playWithPet(const string& petName);
    void walkPet(const string& petName);
    void trainPet(const string& petName);
    void brushPet(const string& petName);
    void bathePet(const string& petName);
    void sleepPet(const string& petName);
    void vetVisitForPet(const string& petName);
    void renamePetByName(const string& oldName, const string& newName);

    /* ---------------- Time & random events ---------------- */
    void advanceTime(int hours);
    void triggerRandomEventForPet(Pet& pet);

    /* ---------------- Achievements ---------------- */
    void checkAchievements();
    void displayAchievements() const;

    /* ---------------- Statistics ---------------- */
    void displayStatistics() const;

    /* ---------------- Persistence (file handling) ---------------- */
    void saveToFile(const string& filename) const;
    void loadFromFile(const string& filename);

    /* ---------------- Getters ---------------- */
    string getName() const { return householdName; }
    int getTotalDays() const { return totalDays; }
    int getCurrentHour() const { return currentHour; }
};

/* ---------------------------- Achievement setup ---------------------------- */

void Household::initAchievements() {
    achievements.clear();
    achievements.push_back({"First Pet", "Adopt your very first pet.", false});
    achievements.push_back({"Five Pets", "Adopt five pets in your household.", false});
    achievements.push_back({"Happy Home", "Have three or more household members.", false});
    achievements.push_back({"Master Trainer", "Complete ten training sessions.", false});
    achievements.push_back({"Healthy Family", "Keep every pet's health above 80.", false});
    achievements.push_back({"Pet Lover", "Perform fifty total care activities.", false});
}

/* ---------------------------- Member management ---------------------------- */

void Household::addMember(const Person& person) {
    members.push_back(person);
    cout << person.getName() << " has joined the " << householdName << " household!\n";
    checkAchievements();
}

void Household::editMember(int index, const string& newName, int newAge, const string& newRole) {
    if (index < 0 || index >= static_cast<int>(members.size()))
        throw HomePawsException("Invalid household member selection.");
    members[index].setName(newName);
    members[index].setAge(newAge);
    members[index].setRole(newRole);
    cout << "Household member updated successfully.\n";
}

void Household::removeMember(int index) {
    if (index < 0 || index >= static_cast<int>(members.size()))
        throw HomePawsException("Invalid household member selection.");
    string removedName = members[index].getName();
    members.erase(members.begin() + index);
    // Clear favoriteOwner references that pointed to the removed member.
    for (auto& petPtr : pets) {
        if (petPtr->getFavoriteOwner() == removedName) petPtr->setFavoriteOwner("");
    }
    cout << removedName << " has left the household.\n";
}

void Household::listMembers() const {
    if (members.empty()) {
        cout << "There are no household members yet.\n";
        return;
    }
    cout << "\nHousehold Members of " << householdName << ":\n";
    for (size_t i = 0; i < members.size(); ++i) {
        cout << " [" << (i + 1) << "] ";
        members[i].display();
    }
}

/* ---------------------------- Pet management ---------------------------- */

void Household::adoptPet(unique_ptr<Pet> newPet) {
    if (findPet(newPet->getName()) != nullptr)
        throw HomePawsException("A pet named \"" + newPet->getName() + "\" already exists. Please choose a unique name.");
    cout << newPet->getName() << " the " << newPet->getSpeciesName()
         << " has been adopted into the " << householdName << " household!\n";
    pets.push_back(move(newPet));
    checkAchievements();
}

void Household::removePet(const string& petName) {
    auto it = find_if(pets.begin(), pets.end(), [&](const unique_ptr<Pet>& p) {
        return p->getName() == petName;
    });
    if (it == pets.end()) throw HomePawsException("No pet named \"" + petName + "\" was found.");
    cout << (*it)->getName() << " has been rehomed. Farewell!\n";
    pets.erase(it);
}

Pet* Household::findPet(const string& petName) {
    string target = petName;
    transform(target.begin(), target.end(), target.begin(), ::tolower);
    for (auto& petPtr : pets) {
        string current = petPtr->getName();
        transform(current.begin(), current.end(), current.begin(), ::tolower);
        if (current == target) return petPtr.get();
    }
    return nullptr;
}

void Household::listPets() const {
    if (pets.empty()) {
        cout << "There are no pets in the household yet. Try adopting one!\n";
        return;
    }
    cout << "\nPets living in " << householdName << ":\n";
    for (const auto& petPtr : pets) {
        petPtr->displaySummary(); // polymorphic call resolved at runtime
    }
}

void Household::viewPet(const string& petName) const {
    for (const auto& petPtr : pets) {
        if (petPtr->getName() == petName) {
            petPtr->displayInfo(); // polymorphic call - Dog/Cat specific fields shown via override
            return;
        }
    }
    throw HomePawsException("No pet named \"" + petName + "\" was found.");
}

/* ---------------------------- Pet care activities ----------------------------
 * Each activity: locates the pet, calls its (possibly overridden) virtual
 * method, updates lifetime statistics, advances simulated time, and checks
 * whether any new achievement has been unlocked.
 * ------------------------------------------------------------------------- */

void Household::feedPet(const string& petName) {
    Pet* pet = findPet(petName);
    if (!pet) throw HomePawsException("No pet named \"" + petName + "\" was found.");
    pet->feed();
    stats.mealsServed++;
    advanceTime(1);
    checkAchievements();
}

void Household::waterPet(const string& petName) {
    Pet* pet = findPet(petName);
    if (!pet) throw HomePawsException("No pet named \"" + petName + "\" was found.");
    pet->giveWater();
    stats.waterGiven++;
    advanceTime(1);
    checkAchievements();
}

void Household::playWithPet(const string& petName) {
    Pet* pet = findPet(petName);
    if (!pet) throw HomePawsException("No pet named \"" + petName + "\" was found.");
    pet->play(); // polymorphic
    stats.gamesPlayed++;
    advanceTime(1);
    checkAchievements();
}

void Household::walkPet(const string& petName) {
    Pet* pet = findPet(petName);
    if (!pet) throw HomePawsException("No pet named \"" + petName + "\" was found.");
    pet->walk(); // polymorphic - Dog gives real walk, base/Cat differ
    stats.walksTaken++;
    advanceTime(2);
    checkAchievements();
}

void Household::trainPet(const string& petName) {
    Pet* pet = findPet(petName);
    if (!pet) throw HomePawsException("No pet named \"" + petName + "\" was found.");
    pet->train(); // polymorphic - Dog can become "trained"
    stats.trainSessions++;
    advanceTime(2);
    checkAchievements();
}

void Household::brushPet(const string& petName) {
    Pet* pet = findPet(petName);
    if (!pet) throw HomePawsException("No pet named \"" + petName + "\" was found.");
    pet->brush();
    stats.brushSessions++;
    advanceTime(1);
    checkAchievements();
}

void Household::bathePet(const string& petName) {
    Pet* pet = findPet(petName);
    if (!pet) throw HomePawsException("No pet named \"" + petName + "\" was found.");
    pet->bathe(); // polymorphic - Cat reacts very differently than Dog
    stats.bathsGiven++;
    advanceTime(1);
    checkAchievements();
}

void Household::sleepPet(const string& petName) {
    Pet* pet = findPet(petName);
    if (!pet) throw HomePawsException("No pet named \"" + petName + "\" was found.");
    pet->sleep();
    stats.sleepSessions++;
    advanceTime(4);
    checkAchievements();
}

void Household::vetVisitForPet(const string& petName) {
    Pet* pet = findPet(petName);
    if (!pet) throw HomePawsException("No pet named \"" + petName + "\" was found.");
    pet->visitVet();
    stats.vetVisits++;
    advanceTime(3);
    checkAchievements();
}

void Household::renamePetByName(const string& oldName, const string& newName) {
    Pet* pet = findPet(oldName);
    if (!pet) throw HomePawsException("No pet named \"" + oldName + "\" was found.");
    if (findPet(newName) != nullptr) throw HomePawsException("That name is already taken by another pet.");
    pet->renamePet(newName);
    cout << oldName << " has been renamed to " << newName << ".\n";
}

/* ---------------------------- Time simulation ----------------------------
 * Every action advances the household clock by a number of hours. When the
 * clock rolls past 24, a new simulated day begins: pets age, daily need
 * decay is applied, and random events may occur.
 * ------------------------------------------------------------------------- */

void Household::advanceTime(int hours) {
    if (hours <= 0) return;
    currentHour += hours;
    while (currentHour >= 24) {
        currentHour -= 24;
        totalDays++;
        cout << "\n[A new day has dawned in " << householdName << ". Day " << totalDays << " begins.]\n";

        for (auto& petPtr : pets) {
            petPtr->agePetByOneDay();
            petPtr->applyDailyDecay(); // polymorphic - Cat overrides hygiene decay

            // Birthday check - every 365 days of age.
            if (petPtr->getAgeInDays() > 0 && petPtr->getAgeInDays() % 365 == 0) {
                cout << "*** Happy Birthday, " << petPtr->getName() << "! Now "
                     << (petPtr->getAgeInDays() / 365) << " year(s) old! ***\n";
            }

            // 35% chance of a random event happening to each pet per day.
            if (Utility::randomInt(1, 100) <= 35) {
                triggerRandomEventForPet(*petPtr);
            }
        }
        checkAchievements();
    }
}

void Household::triggerRandomEventForPet(Pet& pet) {
    // Choose one of several flavorful random events.
    int roll = Utility::randomInt(1, 6);
    switch (roll) {
        case 1: // Illness
            pet.setRawStats(pet.getHunger(), pet.getThirst(),
                             Utility::clampInt(pet.getHealth() - 25),
                             Utility::clampInt(pet.getHappiness() - 10),
                             pet.getEnergy(), pet.getHygiene(), pet.getAffection());
            cout << "[Random Event] " << pet.getName() << " isn't feeling well and may need a vet visit.\n";
            break;
        case 2: // Finding a toy
            pet.setRawStats(pet.getHunger(), pet.getThirst(), pet.getHealth(),
                             Utility::clampInt(pet.getHappiness() + 15),
                             pet.getEnergy(), pet.getHygiene(), pet.getAffection());
            cout << "[Random Event] " << pet.getName() << " found an old toy and is delighted!\n";
            break;
        case 3: // Making a mess
            pet.setRawStats(pet.getHunger(), pet.getThirst(), pet.getHealth(),
                             Utility::clampInt(pet.getHappiness() - 5),
                             pet.getEnergy(), Utility::clampInt(pet.getHygiene() - 15), pet.getAffection());
            cout << "[Random Event] " << pet.getName() << " made a little mess around the house!\n";
            break;
        case 4: { // Learning a trick (dogs only - demonstrates dynamic_cast with polymorphism)
            Dog* dog = dynamic_cast<Dog*>(&pet);
            if (dog != nullptr) {
                cout << "[Random Event] " << pet.getName() << " spontaneously learned a fun new trick!\n";
                pet.setRawStats(pet.getHunger(), pet.getThirst(), pet.getHealth(),
                                 Utility::clampInt(pet.getHappiness() + 10),
                                 pet.getEnergy(), pet.getHygiene(),
                                 Utility::clampInt(pet.getAffection() + 5));
            } else {
                cout << "[Random Event] " << pet.getName() << " seems extra content today.\n";
                pet.setRawStats(pet.getHunger(), pet.getThirst(), pet.getHealth(),
                                 Utility::clampInt(pet.getHappiness() + 8),
                                 pet.getEnergy(), pet.getHygiene(), pet.getAffection());
            }
            break;
        }
        case 5: // Becoming extra happy
            pet.setRawStats(pet.getHunger(), pet.getThirst(), pet.getHealth(),
                             Utility::clampInt(pet.getHappiness() + 20),
                             pet.getEnergy(), pet.getHygiene(), pet.getAffection());
            cout << "[Random Event] " << pet.getName() << " is feeling extra joyful today!\n";
            break;
        case 6: // Feeling lonely
            pet.setRawStats(pet.getHunger(), pet.getThirst(), pet.getHealth(),
                             Utility::clampInt(pet.getHappiness() - 10),
                             pet.getEnergy(), pet.getHygiene(),
                             Utility::clampInt(pet.getAffection() - 15));
            cout << "[Random Event] " << pet.getName() << " is feeling a little lonely and could use attention.\n";
            break;
        default:
            break;
    }
}

/* ---------------------------- Achievements ---------------------------- */

void Household::checkAchievements() {
    // Helper lambda to unlock an achievement by name and announce it if newly unlocked.
    auto unlock = [this](const string& achName) {
        for (auto& a : achievements) {
            if (a.name == achName && !a.unlocked) {
                a.unlocked = true;
                cout << "\n*** ACHIEVEMENT UNLOCKED: " << a.name << " - " << a.description << " ***\n";
            }
        }
    };

    if (petCount() >= 1) unlock("First Pet");
    if (petCount() >= 5) unlock("Five Pets");
    if (memberCount() >= 3) unlock("Happy Home");
    if (stats.trainSessions >= 10) unlock("Master Trainer");
    if (stats.totalActivities() >= 50) unlock("Pet Lover");

    if (!pets.empty()) {
        bool allHealthy = all_of(pets.begin(), pets.end(), [](const unique_ptr<Pet>& p) {
            return p->getHealth() > 80;
        });
        if (allHealthy) unlock("Healthy Family");
    }
}

void Household::displayAchievements() const {
    Utility::printHeader("Achievements - " + householdName);
    for (const auto& a : achievements) {
        cout << " [" << (a.unlocked ? "X" : " ") << "] " << left << setw(16) << a.name
             << " - " << a.description << "\n";
    }
}

/* ---------------------------- Statistics ---------------------------- */

void Household::displayStatistics() const {
    Utility::printHeader("Household Statistics - " + householdName);
    cout << " Total Days Passed     : " << totalDays << "\n";
    cout << " Household Members     : " << memberCount() << "\n";
    cout << " Number of Pets        : " << petCount() << "\n";
    cout << " Meals Served          : " << stats.mealsServed << "\n";
    cout << " Water Given           : " << stats.waterGiven << "\n";
    cout << " Walks Taken           : " << stats.walksTaken << "\n";
    cout << " Games Played          : " << stats.gamesPlayed << "\n";
    cout << " Training Sessions     : " << stats.trainSessions << "\n";
    cout << " Brushing Sessions     : " << stats.brushSessions << "\n";
    cout << " Baths Given           : " << stats.bathsGiven << "\n";
    cout << " Vet Visits            : " << stats.vetVisits << "\n";
    cout << " Sleep Sessions        : " << stats.sleepSessions << "\n";
    cout << " Total Care Activities : " << stats.totalActivities() << "\n";
}

/* ---------------------------- Persistence (File Handling) ----------------------------
 * The household is saved as a simple, human-readable, pipe/comma-delimited
 * text file. Each logical record occupies its own line, tagged with a
 * keyword so loadFromFile() can parse it back unambiguously.
 * ------------------------------------------------------------------------- */

void Household::saveToFile(const string& filename) const {
    ofstream outFile(filename);
    if (!outFile.is_open()) throw HomePawsException("Could not open \"" + filename + "\" for writing.");

    outFile << "HOUSEHOLD|" << householdName << "|" << totalDays << "|" << currentHour << "\n";
    outFile << "STATS|" << stats.serialize() << "\n";

    outFile << "ACH|";
    for (size_t i = 0; i < achievements.size(); ++i) {
        outFile << (achievements[i].unlocked ? "1" : "0");
        if (i + 1 < achievements.size()) outFile << ",";
    }
    outFile << "\n";

    outFile << "MEMBERCOUNT|" << members.size() << "\n";
    for (const auto& m : members) {
        outFile << "MEMBER|" << m.serialize() << "\n";
    }

    outFile << "PETCOUNT|" << pets.size() << "\n";
    for (const auto& petPtr : pets) {
        outFile << "PET|" << petPtr->getSpeciesName() << "|" << petPtr->serializeBase()
                << "|" << petPtr->serializeExtra() << "\n";
    }

    outFile.close();
    cout << "Household \"" << householdName << "\" saved successfully to \"" << filename << "\".\n";
}

void Household::loadFromFile(const string& filename) {
    ifstream inFile(filename);
    if (!inFile.is_open()) throw HomePawsException("Could not open \"" + filename + "\" for reading. Does it exist?");

    // Temporary containers so we don't corrupt current state if loading fails midway.
    string loadedName = householdName;
    int loadedDays = totalDays;
    int loadedHour = currentHour;
    Statistics loadedStats;
    vector<Achievement> loadedAchievements = achievements;
    vector<Person> loadedMembers;
    vector<unique_ptr<Pet>> loadedPets;

    string line;
    while (getline(inFile, line)) {
        if (line.empty()) continue;
        size_t firstPipe = line.find('|');
        if (firstPipe == string::npos) continue;
        string tag = line.substr(0, firstPipe);
        string rest = line.substr(firstPipe + 1);

        if (tag == "HOUSEHOLD") {
            vector<string> parts = Utility::split(rest, '|');
            if (parts.size() < 3) throw HomePawsException("Corrupted HOUSEHOLD record in save file.");
            loadedName = parts[0];
            loadedDays = stoi(parts[1]);
            loadedHour = stoi(parts[2]);
        } else if (tag == "STATS") {
            loadedStats = Statistics::deserialize(rest);
        } else if (tag == "ACH") {
            vector<string> flags = Utility::split(rest, ',');
            for (size_t i = 0; i < loadedAchievements.size() && i < flags.size(); ++i) {
                loadedAchievements[i].unlocked = (flags[i] == "1");
            }
        } else if (tag == "MEMBER") {
            loadedMembers.push_back(Person::deserialize(rest));
        } else if (tag == "PET") {
            vector<string> parts = Utility::split(rest, '|');
            // Expected layout: type|name|breed|gender|personality|age|color|food|toy|owner|
            //                  hunger|thirst|health|happiness|energy|hygiene|affection|extra
            if (parts.size() < 17) throw HomePawsException("Corrupted PET record in save file.");
            string type        = parts[0];
            string petName      = parts[1];
            string petBreed     = parts[2];
            Gender petGender    = stringToGender(parts[3]);
            Personality petPers = stringToPersonality(parts[4]);
            int petAge          = stoi(parts[5]);
            string petColor     = parts[6];
            string petFood      = parts[7];
            string petToy       = parts[8];
            string petOwner     = parts[9];
            int hunger    = stoi(parts[10]);
            int thirst    = stoi(parts[11]);
            int health    = stoi(parts[12]);
            int happiness = stoi(parts[13]);
            int energy    = stoi(parts[14]);
            int hygiene   = stoi(parts[15]);
            int affection = stoi(parts[16]);
            string extra  = (parts.size() > 17) ? parts[17] : "";

            unique_ptr<Pet> newPet;
            if (type == "Dog") {
                vector<string> extraParts = Utility::split(extra, ',');
                bool trained = (!extraParts.empty() && extraParts[0] == "1");
                int walkCount = (extraParts.size() > 1) ? stoi(extraParts[1]) : 0;
                newPet = make_unique<Dog>(petName, petBreed, petGender, petPers, petColor,
                                           petFood, petToy, petOwner, trained, walkCount);
            } else if (type == "Cat") {
                bool indoor = (extra == "1" || extra.empty());
                newPet = make_unique<Cat>(petName, petBreed, petGender, petPers, petColor,
                                           petFood, petToy, petOwner, indoor);
            } else {
                throw HomePawsException("Unknown pet type \"" + type + "\" in save file.");
            }
            newPet->setAgeInDays(petAge);
            newPet->setRawStats(hunger, thirst, health, happiness, energy, hygiene, affection);
            loadedPets.push_back(move(newPet));
        }
        // "MEMBERCOUNT" and "PETCOUNT" tags are informational only and can be skipped.
    }
    inFile.close();

    // Commit the successfully parsed data into this household.
    householdName = loadedName;
    totalDays = loadedDays;
    currentHour = loadedHour;
    stats = loadedStats;
    achievements = loadedAchievements;
    members = move(loadedMembers);
    pets = move(loadedPets);

    cout << "Household \"" << householdName << "\" loaded successfully from \"" << filename << "\".\n";
}

/* ============================================================================
 *  MENU HELPER FUNCTIONS
 *  These free functions build the interactive console menus and translate
 *  user choices into calls on the Household object.
 * ============================================================================
 */

// Lets the user pick a breed from a numbered list, or type a custom one.
string chooseBreed(const vector<string>& breedOptions) {
    for (size_t i = 0; i < breedOptions.size(); ++i) {
        cout << "  " << (i + 1) << ". " << breedOptions[i] << "\n";
    }
    cout << "  " << (breedOptions.size() + 1) << ". Other (type a custom breed)\n";
    int choice = Utility::getValidatedInt("Choose a breed number: ", 1, static_cast<int>(breedOptions.size()) + 1);
    if (choice == static_cast<int>(breedOptions.size()) + 1) {
        return Utility::getValidatedLine("Enter custom breed name: ");
    }
    return breedOptions[choice - 1];
}

Gender choosePetGender() {
    cout << "  1. Male\n  2. Female\n";
    int choice = Utility::getValidatedInt("Choose gender: ", 1, 2);
    return (choice == 1) ? Gender::Male : Gender::Female;
}

Personality choosePersonality() {
    cout << "  1. Playful\n  2. Lazy\n  3. Friendly\n  4. Shy\n  5. Curious\n  6. Protective\n";
    int choice = Utility::getValidatedInt("Choose personality: ", 1, 6);
    switch (choice) {
        case 1: return Personality::Playful;
        case 2: return Personality::Lazy;
        case 3: return Personality::Friendly;
        case 4: return Personality::Shy;
        case 5: return Personality::Curious;
        default: return Personality::Protective;
    }
}

// Guides the user through adopting a brand-new pet and adds it to the household.
void handleAdoptPet(Household& household) {
    Utility::printHeader("Adopt a New Pet");
    cout << "  1. Dog\n  2. Cat\n";
    int typeChoice = Utility::getValidatedInt("What kind of pet would you like to adopt? ", 1, 2);

    string name = Utility::getValidatedLine("Enter the pet's name: ");
    string breed = (typeChoice == 1) ? chooseBreed(dogBreedList()) : chooseBreed(catBreedList());
    Gender gender = choosePetGender();
    Personality personality = choosePersonality();
    string color = Utility::getValidatedLine("Enter the pet's color: ");
    string food = Utility::getValidatedLine("Enter the pet's favorite food: ");
    string toy = Utility::getValidatedLine("Enter the pet's favorite toy: ");

    string owner = "";
    if (household.memberCount() > 0) {
        household.listMembers();
        cout << "  0. None for now\n";
        int ownerChoice = Utility::getValidatedInt("Choose a favorite owner by number: ", 0, household.memberCount());
        if (ownerChoice > 0) owner = household.getMembers()[ownerChoice - 1].getName();
    } else {
        cout << "(No household members yet, so this pet won't have a favorite owner for now.)\n";
    }

    unique_ptr<Pet> newPet;
    if (typeChoice == 1) {
        newPet = make_unique<Dog>(name, breed, gender, personality, color, food, toy, owner);
    } else {
        newPet = make_unique<Cat>(name, breed, gender, personality, color, food, toy, owner);
    }
    household.adoptPet(move(newPet));
}

// Asks for a pet name and returns it, verifying at least one pet exists first.
string promptExistingPetName(Household& household, const string& actionLabel) {
    if (household.petCount() == 0) throw HomePawsException("There are no pets in the household yet.");
    household.listPets();
    return Utility::getValidatedLine("Enter the name of the pet to " + actionLabel + ": ");
}

// Sub-menu: manage household members (add / edit / remove / list).
void manageHouseholdMembersMenu(Household& household) {
    bool inSubMenu = true;
    while (inSubMenu) {
        Utility::printHeader("Manage Household Members - " + household.getName());
        cout << " 1. Add a Member\n";
        cout << " 2. Edit a Member\n";
        cout << " 3. Remove a Member\n";
        cout << " 4. List All Members\n";
        cout << " 5. Back to Main Menu\n";
        int choice = Utility::getValidatedInt("Enter your choice: ", 1, 5);

        try {
            switch (choice) {
                case 1: {
                    string name = Utility::getValidatedLine("Enter member's name: ");
                    int age = Utility::getValidatedInt("Enter member's age: ", 0, 130);
                    string role = Utility::getValidatedLine("Enter member's role (e.g. Parent, Child, Roommate): ");
                    household.addMember(Person(name, age, role));
                    break;
                }
                case 2: {
                    household.listMembers();
                    if (household.memberCount() == 0) break;
                    int idx = Utility::getValidatedInt("Enter the member number to edit: ", 1, household.memberCount());
                    string name = Utility::getValidatedLine("Enter new name: ");
                    int age = Utility::getValidatedInt("Enter new age: ", 0, 130);
                    string role = Utility::getValidatedLine("Enter new role: ");
                    household.editMember(idx - 1, name, age, role);
                    break;
                }
                case 3: {
                    household.listMembers();
                    if (household.memberCount() == 0) break;
                    int idx = Utility::getValidatedInt("Enter the member number to remove: ", 1, household.memberCount());
                    household.removeMember(idx - 1);
                    break;
                }
                case 4:
                    household.listMembers();
                    break;
                case 5:
                    inSubMenu = false;
                    break;
            }
        } catch (const HomePawsException& ex) {
            cout << "Error: " << ex.what() << "\n";
        }

        if (inSubMenu) Utility::pause();
    }
}

// Sub-menu: perform a care activity on a chosen pet.
void petCareActivitiesMenu(Household& household) {
    bool inSubMenu = true;
    while (inSubMenu) {
        Utility::printHeader("Pet Care Activities - " + household.getName());
        cout << " 1. Feed\n";
        cout << " 2. Give Water\n";
        cout << " 3. Play\n";
        cout << " 4. Walk (best for dogs)\n";
        cout << " 5. Train\n";
        cout << " 6. Brush\n";
        cout << " 7. Bathe\n";
        cout << " 8. Sleep\n";
        cout << " 9. Visit Vet\n";
        cout << " 10. Rename Pet\n";
        cout << " 11. Back to Main Menu\n";
        int choice = Utility::getValidatedInt("Enter your choice: ", 1, 11);

        try {
            if (choice == 11) { inSubMenu = false; continue; }

            if (choice == 10) {
                string oldName = promptExistingPetName(household, "rename");
                string newName = Utility::getValidatedLine("Enter the new name: ");
                household.renamePetByName(oldName, newName);
            } else {
                string label;
                switch (choice) {
                    case 1: label = "feed"; break;
                    case 2: label = "give water to"; break;
                    case 3: label = "play with"; break;
                    case 4: label = "walk"; break;
                    case 5: label = "train"; break;
                    case 6: label = "brush"; break;
                    case 7: label = "bathe"; break;
                    case 8: label = "put to sleep"; break;
                    case 9: label = "take to the vet"; break;
                    default: label = "care for"; break;
                }
                string petName = promptExistingPetName(household, label);
                switch (choice) {
                    case 1: household.feedPet(petName); break;
                    case 2: household.waterPet(petName); break;
                    case 3: household.playWithPet(petName); break;
                    case 4: household.walkPet(petName); break;
                    case 5: household.trainPet(petName); break;
                    case 6: household.brushPet(petName); break;
                    case 7: household.bathePet(petName); break;
                    case 8: household.sleepPet(petName); break;
                    case 9: household.vetVisitForPet(petName); break;
                }
            }
        } catch (const HomePawsException& ex) {
            cout << "Error: " << ex.what() << "\n";
        }

        if (inSubMenu) Utility::pause();
    }
}

/* ============================================================================
 *  MAIN MENU AND PROGRAM ENTRY POINT
 * ============================================================================
 */

// Prompts for a save file name, defaulting to a standard filename if left blank.
string promptFilename() {
    string filename = Utility::getValidatedLine(
        "Enter file name (leave blank for \"homepaws_save.txt\"): ", true);
    return filename.empty() ? "homepaws_save.txt" : filename;
}

void printMainMenu(const Household& household) {
    Utility::printHeader("HomePaws - " + household.getName() +
                          "  |  Day " + to_string(household.getTotalDays()) +
                          ", Hour " + to_string(household.getCurrentHour()) + ":00");
    cout << " 1.  Manage Household Members\n";
    cout << " 2.  Adopt a New Pet\n";
    cout << " 3.  View All Pets\n";
    cout << " 4.  View One Pet (Full Details)\n";
    cout << " 5.  Pet Care Activities\n";
    cout << " 6.  Remove a Pet\n";
    cout << " 7.  Pass Time (advance a few hours)\n";
    cout << " 8.  View Statistics\n";
    cout << " 9.  View Achievements\n";
    cout << " 10. Save Household\n";
    cout << " 11. Load Household\n";
    cout << " 12. Exit\n";
}

int main() {
    cout << "============================================================\n";
    cout << "      Welcome to HomePaws - Your Cozy Pet Household!\n";
    cout << "============================================================\n";

    string householdName = Utility::getValidatedLine("Enter a name for your household: ");
    Household household(householdName);

    bool running = true;
    while (running) {
        printMainMenu(household);
        int choice = Utility::getValidatedInt("Enter your choice: ", 1, 12);

        try {
            switch (choice) {
                case 1:
                    manageHouseholdMembersMenu(household);
                    break;

                case 2:
                    handleAdoptPet(household);
                    break;

                case 3:
                    household.listPets();
                    break;

                case 4: {
                    string petName = promptExistingPetName(household, "view");
                    household.viewPet(petName);
                    break;
                }

                case 5:
                    petCareActivitiesMenu(household);
                    break;

                case 6: {
                    string petName = promptExistingPetName(household, "remove");
                    household.removePet(petName);
                    break;
                }

                case 7: {
                    int hours = Utility::getValidatedInt("How many hours should pass (1-48)? ", 1, 48);
                    household.advanceTime(hours);
                    break;
                }

                case 8:
                    household.displayStatistics();
                    break;

                case 9:
                    household.displayAchievements();
                    break;

                case 10: {
                    string filename = promptFilename();
                    household.saveToFile(filename);
                    break;
                }

                case 11: {
                    string filename = promptFilename();
                    household.loadFromFile(filename);
                    break;
                }

                case 12:
                    cout << "\nThank you for playing HomePaws! Your pets will miss you. Goodbye!\n";
                    running = false;
                    break;
            }
        } catch (const HomePawsException& ex) {
            // Handles all application-specific errors gracefully (invalid pet names,
            // bad indices, file I/O failures, etc.) without crashing the program.
            cout << "\n[HomePaws Error] " << ex.what() << "\n";
        } catch (const exception& ex) {
            // Safety net for any other standard exceptions (e.g. stoi conversion errors
            // from a corrupted save file).
            cout << "\n[Unexpected Error] " << ex.what() << "\n";
        }

        if (running && choice != 12) Utility::pause();
    }

    return 0;
}
