class hal_updatable {
    private:
        bool instanciated = false;

    public:
        virtual bool begin() = 0;
        virtual void update() = 0;

        bool isInstanciated() const {
            return instanciated;
        }

        void setInstanciated(bool value) {
            instanciated = value;
        }
};