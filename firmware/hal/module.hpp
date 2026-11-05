class hal_module {
    private:
        bool instanciated = false;

    public:
        virtual bool begin() = 0;
        
        bool is_instanciated() const {
            return instanciated;
        }

        void set_instanciated(bool value) {
            instanciated = value;
        }
};