class LASPManager : public cSimpleModule {
  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    void runPlacementStrategy();

    std::string strategy;
    simtime_t lastUpdate;
};
