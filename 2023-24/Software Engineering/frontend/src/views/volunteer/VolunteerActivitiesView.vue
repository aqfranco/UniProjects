<template>
  <div>
    <v-card class="table">
      <v-data-table
        :headers="headers"
        :items="activities"
        :search="search"
        disable-pagination
        :hide-default-footer="true"
        :mobile-breakpoint="0"
        data-cy="volunteerActivitiesTable"
      >
        <template v-slot:top>
          <v-card-title>
            <v-text-field
              v-model="search"
              append-icon="search"
              label="Search"
              class="mx-2"
            />
            <v-spacer />
          </v-card-title>
        </template>
        <template v-slot:[`item.themes`]="{ item }">
          <v-chip v-for="theme in item.themes" v-bind:key="theme.id">
            {{ theme.completeName }}
          </v-chip>
        </template>
        <template v-slot:[`item.action`]="{ item }">
          <v-tooltip v-if="item.state === 'APPROVED'" bottom>
            <template v-slot:activator="{ on }">
              <v-icon
                class="mr-2 action-button"
                color="red"
                v-on="on"
                data-cy="reportButton"
                @click="reportActivity(item)"
                >warning</v-icon
              >
            </template>
            <span>Report Activity</span>
          </v-tooltip>
          <v-tooltip v-if="checkAssessment(item)" bottom>
            <template v-slot:activator="{ on }">
              <v-icon
                class="mr-2 action-button"
                color="blue"
                v-on="on"
                data-cy="assessButton"
                @click="createAssessment(item)"
                >rate_review</v-icon
              >
            </template>
            <span>Write Assessment</span>
          </v-tooltip>
          <v-tooltip
            v-if="!checkIfVolunteerEnrolled(item) && checkBeforeDeadline(item)"
            bottom
          >
            <template v-slot:activator="{ on }">
              <v-icon
                class="mr-2 action-button"
                color="blue"
                v-on="on"
                data-cy="applyActivityButton"
                @click="createEnrollment(item)"
                >login</v-icon
              >
            </template>
            <span>Apply For Activity</span>
          </v-tooltip>
        </template>
      </v-data-table>
      <volunteer-create-assessment
        v-if="institution && createAssessmentDialog"
        v-model="createAssessmentDialog"
        :institution="institution"
        v-on:assessment-created="onCreatedAssessment"
        v-on:close-dialog="onCloseDialog"
      />
      <enrollment-dialog
        v-if="!!newEnrollment && createEnrollmentDialog && !!currentActivity"
        v-model="createEnrollmentDialog"
        :enrollment="newEnrollment"
        :activity="currentActivity"
        v-on:save-enrollment="onSaveEnrollment"
        v-on:close-enrollment-dialog="onCloseEnrollmentDialog"
      />
    </v-card>
  </div>
</template>

<script lang="ts">
import { Component, Vue } from 'vue-property-decorator';
import RemoteServices from '@/services/RemoteServices';
import Activity from '@/models/activity/Activity';
import { show } from 'cli-cursor';
import Institution from '@/models/institution/Institution';
import VolunteerCreateAssessment from '@/views/volunteer/VolunteerCreateAssessment.vue';
import EnrollmentDialog from '@/views/volunteer/EnrollmentDialog.vue';
import Enrollment from '@/models/enrollment/Enrollment';

@Component({
  methods: { show },
  components: {
    'volunteer-create-assessment': VolunteerCreateAssessment,
    'enrollment-dialog': EnrollmentDialog,
  },
})
export default class VolunteerActivitiesView extends Vue {
  activities: Activity[] = [];
  institution: Institution = new Institution();
  validActivities: Activity[] = [];
  createAssessmentDialog: boolean = false;
  search: string = '';
  volunteerEnrollments: Enrollment[] = [];

  newEnrollment: Enrollment | null = null;
  currentActivity: Activity | null = null;
  createEnrollmentDialog: boolean = false;
  headers: object = [
    {
      text: 'Name',
      value: 'name',
      align: 'left',
      width: '5%',
    },
    {
      text: 'Region',
      value: 'region',
      align: 'left',
      width: '5%',
    },
    {
      text: 'Participants',
      value: 'participantsNumberLimit',
      align: 'left',
      width: '5%',
    },
    {
      text: 'Themes',
      value: 'themes',
      align: 'left',
      width: '5%',
    },
    {
      text: 'Description',
      value: 'description',
      align: 'left',
      width: '30%',
    },
    {
      text: 'State',
      value: 'state',
      align: 'left',
      width: '5%',
    },
    {
      text: 'Start Date',
      value: 'formattedStartingDate',
      align: 'left',
      width: '5%',
    },
    {
      text: 'End Date',
      value: 'formattedEndingDate',
      align: 'left',
      width: '5%',
    },
    {
      text: 'Application Deadline',
      value: 'formattedApplicationDeadline',
      align: 'left',
      width: '5%',
    },
    {
      text: 'Actions',
      value: 'action',
      align: 'left',
      sortable: false,
      width: '5%',
    },
  ];

  async created() {
    await this.$store.dispatch('loading');
    try {
      this.activities = await RemoteServices.getActivities();
      this.volunteerEnrollments = await RemoteServices.getVolunteerEnrollments(
        this.$store.getters.getUser.id,
      );
    } catch (error) {
      await this.$store.dispatch('error', error);
    }
    await this.$store.dispatch('clearLoading');
    for (let activity of this.activities) {
      await this.checkActivity(activity);
    }
  }

  async reportActivity(activity: Activity) {
    if (activity.id !== null) {
      try {
        const result = await RemoteServices.reportActivity(
          this.$store.getters.getUser.id,
          activity.id,
        );
        this.activities = this.activities.filter((a) => a.id !== activity.id);
        this.activities.unshift(result);
      } catch (error) {
        await this.$store.dispatch('error', error);
      }
    }
  }

  checkIfVolunteerEnrolled(activity: Activity) {
    if (activity.id !== null) {
      try {
        // Check if the volunteer is enrolled in the activity. Return true if enrolled, false otherwise
        return this.volunteerEnrollments.some(
          (enrollment) => enrollment.activityId === activity.id,
        );
      } catch (error) {
        this.$store.dispatch('error', error);
        return false; // Return false if an error occurs
      }
    }
    return false; // Return false if activity ID is null
  }

  checkBeforeDeadline(activity: Activity) {
    if (activity.id !== null) {
      try {
        const deadlineDate = new Date(activity.applicationDeadline);
        const nowDate = new Date();
        return deadlineDate > nowDate;
      } catch (error) {
        this.$store.dispatch('error', error);
        return false; // Return false if an error occurs
      }
    }
    return false;
  }

  createEnrollment(activity: Activity) {
    console.log('[createEnrollment]: begin Enrollment dialog\n');
    this.currentActivity = activity;
    this.newEnrollment = new Enrollment();
    this.createEnrollmentDialog = true;
    console.log(
      '[createEnrollment]: \n\tcurActivity:' +
        this.currentActivity +
        '\n\tnewEnrollment: ' +
        this.newEnrollment +
        '\n\tEnrollDialog: ' +
        this.createEnrollmentDialog,
    );
  }
  onCloseEnrollmentDialog() {
    this.currentActivity = null;
    this.newEnrollment = null;
    this.createEnrollmentDialog = false;
  }
  async onSaveEnrollment(enrollment: Enrollment) {
    this.volunteerEnrollments.unshift(enrollment);
    this.createEnrollmentDialog = false;
    this.currentActivity = null;
    this.newEnrollment = null;
  }

  async onCloseDialog() {
    this.createAssessmentDialog = false;
  }

  async onCreatedAssessment() {
    this.createAssessmentDialog = false;
    this.validActivities = this.validActivities.filter(
      (activity) => activity.institution.id !== this.institution.id,
    );
  }

  createAssessment(activity: Activity) {
    this.institution.id = activity.institution.id;
    this.createAssessmentDialog = true;
  }

  async checkActivity(activity: Activity) {
    // Check if the activity has already finished
    if (activity.endingDate > new Date().toISOString()) {
      return;
    }
    // Check if the volunteer has participated on some activity by this institution
    const participating = (
      await RemoteServices.getVolunteerParticipations()
    ).some((participation) => participation.activityId === activity.id);

    if (!participating) {
      return;
    }
    // Check if the volunteer has already assessed this institution
    const set = new Set(await RemoteServices.getVolunteerAssessments());
    const assessments = (
      await RemoteServices.getInstitutionAssessments(activity.institution.id)
    ).every((assessment) => set.has(assessment));

    if (!assessments) {
      return;
    }

    this.validActivities.push(activity);
  }

  checkAssessment(activity: Activity): boolean {
    return this.validActivities.includes(activity);
  }
}
</script>

<style lang="scss" scoped></style>
